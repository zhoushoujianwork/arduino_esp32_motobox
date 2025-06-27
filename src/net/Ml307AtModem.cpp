/*
 * ml307 AT Modem 驱动实现
 * 专注于LBS定位功能
 */

#include "Ml307AtModem.h"

#ifdef ENABLE_GSM
Ml307AtModem ml307_at(Serial1, GSM_RX_PIN, GSM_TX_PIN);
#else
Ml307AtModem ml307_at(Serial1, 16, 17);
#endif

Ml307AtModem::Ml307AtModem(HardwareSerial &serial, int rxPin, int txPin)
    : _serial(serial), _rxPin(rxPin), _txPin(txPin), _lbsEnabled(false), _debug(false), 
      _isNetworkReady(false), _lastNetworkReadyCheckTime(0), _lastLBSUpdate(0), 
      _isLBSLoading(false), _lastLBSLocation(""), _lastCMEErrorTime(0), _cmeErrorCount(0), _backoffDelay(5000)
{
}

bool Ml307AtModem::begin(uint32_t baudrate)
{
    Serial.println("开始初始化ml307模块,波特率:" + String(baudrate));

    // 使用固定波特率初始化
    _serial.begin(baudrate, SERIAL_8N1, _rxPin, _txPin);
    delay(1000); // 等待模块稳定

    // 清空缓冲区
    flushInput();

    while (!isReady())
    {
        debugPrint("等待模块就绪...");
        delay(2000);
    }

    // 必须成功初始化
    while (!initModem())
    {
        debugPrint("重试初始化模块...");
        delay(1000);
    }

    return true;
}

bool Ml307AtModem::tryBaudrate(uint32_t baudrate)
{
    _serial.begin(baudrate, SERIAL_8N1, _rxPin, _txPin);
    delay(100);

    // 清空缓冲区
    while (_serial.available())
    {
        _serial.read();
    }

    // 多次尝试AT测试
    for (int i = 0; i < 3; i++)
    {
        debugPrint("> AT");
        _serial.println("AT");

        String response = waitResponse(1000);
        debugPrint("< " + response);

        if (response.indexOf("OK") >= 0)
        {
            return true;
        }
        delay(100);
    }

    return false;
}

bool Ml307AtModem::sendATWithRetry(const String &cmd, const String &expected, int maxRetry, uint32_t timeout)
{
    for (int i = 0; i < maxRetry; ++i)
    {
        if (sendAT(cmd, expected, timeout))
        {
            return true;
        }
        delay(500);
    }
    return false;
}

bool Ml307AtModem::initModem()
{
    delay(2000);

    // 关闭回显
    if (!sendATWithRetry("ATE0"))
    {
        debugPrint("关闭回显失败");
        return false;
    }

    // 启用网络注册状态主动上报
    if (!sendATWithRetry("AT+CEREG=1"))
    {
        debugPrint("设置CEREG失败");
        return false;
    }

    // 检查 PIN 码状态
    int pinRetry = 0;
    const int maxPinRetry = 5;
    String pinStatus;
    do
    {
        pinStatus = sendATWithResponse("AT+CPIN?");
        if (pinStatus.indexOf("READY") >= 0)
            break;
        delay(500);
        pinRetry++;
    } while (pinRetry < maxPinRetry);
    if (pinStatus.indexOf("READY") < 0)
    {
        debugPrint("SIM卡 PIN 码未就绪");
        return false;
    }

    // 检查信号强度
    int csqRetry = 0;
    int csq = -1;
    do
    {
        csq = getCSQ();
        if (csq >= 5)
            break;
        delay(1000);
        csqRetry++;
    } while (csqRetry < 5);
    if (csq < 5)
    {
        debugPrint("信号太弱，无法注册网络");
        return false;
    }

    // 激活PDP上下文
    if (!sendATWithRetry("AT+MIPCALL?", "OK", 5, 5000))
    {
        debugPrint("激活PDP上下文失败");
        return false;
    }

    // 等待网络注册
    while (!isNetworkReadyCheck())
    {
        debugPrint("等待网络注册...");
        delay(2000);
    }

    return true;
}

bool Ml307AtModem::isNetworkReadyCheck()
{
    if (millis() - _lastNetworkReadyCheckTime > 10000) {
        _lastNetworkReadyCheckTime = millis();
    } else {
        return _isNetworkReady;
    }

    String pdpResponse = sendATWithResponse("AT+CEREG?");
    debugPrint("CEREG状态: " + pdpResponse);

    // 查找状态码
    int idx = pdpResponse.indexOf("+CEREG:");
    if (idx >= 0)
    {
        idx = pdpResponse.indexOf(',', idx);
        if (idx >= 0 && pdpResponse.length() > idx + 1)
        {
            char stat = pdpResponse.charAt(idx + 1);
            String statusMsg;
            switch (stat)
            {
            case '0':
                statusMsg = "未注册，未搜索新运营商";
                _isNetworkReady = false;
                break;
            case '1':
                statusMsg = "已注册，本地网";
                _isNetworkReady = true;
                break;
            case '2':
                statusMsg = "未注册，正在搜索新运营商";
                _isNetworkReady = false;
                break;
            case '3':
                statusMsg = "注册被拒绝";
                _isNetworkReady = false;
                break;
            case '4':
                statusMsg = "未知（超出覆盖范围）";
                _isNetworkReady = false;
                break;
            case '5':
                statusMsg = "已注册，漫游网";
                _isNetworkReady = true;
                break;
            default:
                statusMsg = "未知状态";
                _isNetworkReady = false;
                break;
            }
            debugPrint("CEREG注册状态: " + String(stat) + " - " + statusMsg);
            return stat == '1' || stat == '5';
        }
    }

    debugPrint("CEREG响应格式异常");
    return false;
}

bool Ml307AtModem::reset()
{
    return sendAT("AT+CFUN=1,1", "OK", 10000);
}

String Ml307AtModem::getLocalIP()
{
    String response = sendATWithResponse("AT+CGPADDR=1");
    int start = response.indexOf("+CGPADDR: 1,\"");
    if (start >= 0)
    {
        start += 12;
        int end = response.indexOf("\"", start);
        if (end >= 0)
        {
            return response.substring(start, end);
        }
    }
    return "";
}

int Ml307AtModem::getCSQ()
{
    String response = sendATWithResponse("AT+CSQ");
    int start = response.indexOf("+CSQ: ");
    if (start >= 0)
    {
        start += 6;
        return response.substring(start).toInt();
    }
    return -1;
}

String Ml307AtModem::getIMEI()
{
    return sendATWithResponse("AT+CGSN");
}

String Ml307AtModem::getICCID()
{
    String response = sendATWithResponse("AT+CCID");
    int start = response.indexOf("+CCID: ");
    if (start >= 0)
    {
        start += 7;
        return response.substring(start);
    }
    return "";
}

String Ml307AtModem::getModuleName()
{
    return sendATWithResponse("AT+CGMM");
}

String Ml307AtModem::getCarrierName()
{
    String response = sendATWithResponse("AT+COPS?");
    int start = response.indexOf("+COPS: 0,0,\"");
    if (start >= 0)
    {
        start += 11;
        int end = response.indexOf("\"", start);
        if (end >= 0)
        {
            return response.substring(start, end);
        }
    }
    return "";
}

bool Ml307AtModem::sendAT(const String &cmd, const String &expected, uint32_t timeout)
{
    flushInput();
    debugPrint("> " + cmd);
    _serial.println(cmd);
    return expectResponse(expected, timeout);
}

String Ml307AtModem::sendATWithResponse(const String &cmd, uint32_t timeout)
{
    flushInput();
    debugPrint("> " + cmd);
    _serial.println(cmd);
    return waitResponse(timeout);
}

bool Ml307AtModem::isReady()
{
    String response = sendATWithResponse("AT");
    if (response.indexOf("OK") >= 0)
    {
        return true;
    }
    if (response.indexOf("ml307") >= 0 ||
        response.indexOf("4G_DTU_TCP") >= 0 ||
        response.indexOf("PROD:") >= 0)
    {
        debugPrint("模块已就绪（返回模块信息）");
        return true;
    }
    return false;
}

void Ml307AtModem::flushInput()
{
    while (_serial.available())
    {
        _serial.read();
    }
}

String Ml307AtModem::waitResponse(uint32_t timeout)
{
    String response;
    uint32_t start = millis();
    bool hasError = false;
    bool hasMLBSLOC = false;

    while (millis() - start < timeout)
    {
        if (_serial.available())
        {
            char c = _serial.read();
            response += c;

            if (response.endsWith("\r\n"))
            {
                if (response.indexOf("+MLBSLOC:") >= 0)
                {
                    hasMLBSLOC = true;
                    debugPrint("检测到LBS位置信息: " + response);
                }

                if (response.indexOf("+CME ERROR:") >= 0)
                {
                    hasError = true;
                    debugPrint("检测到CME错误: " + response);
                }

                if (response.endsWith("\r\nOK\r\n") ||
                    response.endsWith("\r\nERROR\r\n") ||
                    response.endsWith("\r\n> ") ||
                    response.endsWith("\"CONNECT\"\r\n"))
                {
                    break;
                }

                if (hasError && hasMLBSLOC)
                {
                    debugPrint("收到错误和MLBSLOC，结束等待");
                    break;
                }
            }
        }
        delay(1);
    }

    debugPrint("完整AT响应: [" + response + "]");
    return response;
}

bool Ml307AtModem::expectResponse(const String &expected, uint32_t timeout)
{
    String response = waitResponse(timeout);
    return response.indexOf(expected) >= 0;
}

void Ml307AtModem::setDebug(bool debug)
{
    _debug = debug;
}

void Ml307AtModem::debugPrint(const String &msg)
{
    if (_debug)
    {
        Serial.println("[ml307atmodem] [debug] " + msg);
    }
}

// ==================== LBS 功能实现 ====================

bool Ml307AtModem::enableLBS(bool enable)
{
    debugPrint("配置LBS: " + String(enable ? "启用" : "禁用"));
    if (enable)
    {
        debugPrint("=== OneOs定位流程（ML307R-DL专用）===");
        
        // 1. 设置平台为OneOs（40）
        if (!sendATWithRetry("AT+MLBSCFG=\"method\",40", "OK", 3, 2000))
        {
            debugPrint("OneOs定位平台配置失败");
            return false;
        }
        debugPrint("OneOs定位平台配置成功");

        // 2. 设置PID（可选）
        if (sendATWithRetry("AT+MLBSCFG=\"pid\",\"\"", "OK", 3, 2000)) {
            debugPrint("OneOs PID配置成功");
        }
        
        // 3. 启用邻区定位（可选）
        if (sendATWithRetry("AT+MLBSCFG=\"nearbtsen\",1", "OK", 3, 2000)) {
            debugPrint("邻区定位配置成功");
        }

        // 4. 设置精度
        if (sendATWithRetry("AT+MLBSCFG=\"precision\",8", "OK", 3, 2000)) {
            debugPrint("精度配置成功");
        }

        _lbsEnabled = true;
        _lastLBSUpdate = 0;
        debugPrint("LBS配置完成，可以开始定位");
        return true;
    }
    else
    {
        _lbsEnabled = false;
        return true;
    }
}

bool Ml307AtModem::isLBSEnabled()
{
    String response = sendATWithResponse("AT+MLBSCFG=\"method\"");
    return response.indexOf("+MLBSCFG: \"method\",40") >= 0;
}

bool Ml307AtModem::updateLBSData()
{
    if (_isLBSLoading) {
        debugPrint("LBS正在加载中，跳过本次请求");
        return false;
    }
    
    _isLBSLoading = true;
    
    if (!_lbsEnabled)
    {
        debugPrint("LBS未启用");
        _isLBSLoading = false;
        return false;
    }
    
    // 检查CME错误退避
    if (shouldSkipLBSRequest()) {
        _isLBSLoading = false;
        return false;
    }
    
    // 统一使用10秒间隔，与GPSManager保持一致
    if (_lastLBSUpdate > 0 && (millis() - _lastLBSUpdate) < 10000) // 10秒间隔
    {
        _isLBSLoading = false;
        return false;
    }
    
    // 检查网络状态
    if (!isNetworkReady())
    {
        debugPrint("网络未就绪，无法进行LBS定位");
        _isLBSLoading = false;
        return false;
    }
    
    // 检查信号强度
    int csq = getCSQ();
    if (csq < 10)
    {
        debugPrint("信号太弱，CSQ: " + String(csq));
        _isLBSLoading = false;
        return false;
    }
    
    // 优化LBS请求策略：只尝试一次，但增加等待时间
    debugPrint("发送LBS定位请求");
    
    String response = sendATWithResponseThreadSafe("AT+MLBSLOC", 15000); // 增加超时时间到15秒
    debugPrint("LBS响应: " + response);
    
    bool success = false;
    
    // 检查是否包含位置数据
    if (response.indexOf("+MLBSLOC: 100,") >= 0) {
        success = true;
        resetCMEErrorCount(); // 成功时重置错误计数
        debugPrint("LBS定位成功，包含位置数据");
    } else if (response.indexOf("+CME ERROR: 105") >= 0) {
        handleCMEError(); // 处理CME错误
        debugPrint("LBS服务忙，进入退避模式");
    } else if (response.indexOf("OK") >= 0 && response.indexOf("+MLBSLOC:") < 0) {
        // 只返回OK但没有位置数据，这是正常现象，不需要重试
        debugPrint("LBS请求已发送，等待位置数据返回");
        success = false; // 不标记为成功，但也不增加错误计数
    } else {
        debugPrint("LBS响应异常: " + response);
    }
    
    if (success) {
        _lastLBSLocation = response;
        _lastLBSUpdate = millis();
        
        if (parseLBSResponse(response)) {
            debugPrint("LBS数据解析成功");
        } else {
            debugPrint("LBS数据解析失败");
            success = false;
        }
    } else {
        debugPrint("LBS定位失败");
    }
    
    _isLBSLoading = false;
    return success;
}

bool Ml307AtModem::parseLBSResponse(const String& response)
{
    int start = response.indexOf("+MLBSLOC: ");
    if (start < 0) {
        debugPrint("未找到LBS位置信息");
        return false;
    }
    
    start += 10;
    int end = response.indexOf("\r\n", start);
    if (end < 0) {
        end = response.length();
    }
    
    String data = response.substring(start, end);
    debugPrint("解析LBS数据: " + data);
    
    int comma1 = data.indexOf(',');
    int comma2 = data.indexOf(',', comma1 + 1);
    
    if (comma1 > 0 && comma2 > 0) {
        int stat = data.substring(0, comma1).toInt();
        
        if (stat == 100) {
            float longitude = data.substring(comma1 + 1, comma2).toFloat();
            float latitude = data.substring(comma2 + 1).toFloat();
            
            if (longitude >= -180 && longitude <= 180 && 
                latitude >= -90 && latitude <= 90) {
                
                extern lbs_data_t lbs_data;
                lbs_data.valid = true;
                lbs_data.longitude = longitude;
                lbs_data.latitude = latitude;
                lbs_data.stat = stat;
                lbs_data.state = LBSState::DONE;
                lbs_data.timestamp = millis();
                
                // 尝试解析半径（如果存在）
                int comma3 = data.indexOf(',', comma2 + 1);
                if (comma3 > 0) {
                    lbs_data.radius = data.substring(comma3 + 1).toInt();
                    
                    // 尝试解析描述信息（如果存在）
                    int comma4 = data.indexOf(',', comma3 + 1);
                    if (comma4 > 0) {
                        lbs_data.descr = data.substring(comma4 + 1);
                    }
                }
                
                debugPrint("LBS数据解析成功 - 经度: " + String(longitude, 6) + 
                          ", 纬度: " + String(latitude, 6) + 
                          ", 半径: " + String(lbs_data.radius) + "m");
                return true;
            } else {
                debugPrint("经纬度超出有效范围");
                return false;
            }
        } else {
            debugPrint("LBS定位失败，状态码: " + String(stat));
            return false;
        }
    } else {
        debugPrint("LBS数据格式错误");
        return false;
    }
}

String Ml307AtModem::getLBSRawData()
{
    return _lastLBSLocation;
}

lbs_data_t Ml307AtModem::getLBSData()
{
    extern lbs_data_t lbs_data;
    return lbs_data;
}

bool Ml307AtModem::isLBSDataValid()
{
    extern lbs_data_t lbs_data;
    return lbs_data.valid && 
           lbs_data.longitude >= -180 && lbs_data.longitude <= 180 &&
           lbs_data.latitude >= -90 && lbs_data.latitude <= 90;
}

// ==================== 线程安全方法 ====================

bool Ml307AtModem::sendATThreadSafe(const String &cmd, const String &expected, uint32_t timeout)
{
    std::lock_guard<std::mutex> lock(_atMutex);
    return sendAT(cmd, expected, timeout);
}

String Ml307AtModem::sendATWithResponseThreadSafe(const String &cmd, uint32_t timeout)
{
    std::lock_guard<std::mutex> lock(_atMutex);
    return sendATWithResponse(cmd, timeout);
}

bool Ml307AtModem::sendATWithRetryThreadSafe(const String &cmd, const String &expected, int maxRetry, uint32_t timeout)
{
    std::lock_guard<std::mutex> lock(_atMutex);
    for (int i = 0; i < maxRetry; ++i)
    {
        if (sendAT(cmd, expected, timeout))
        {
            return true;
        }
        delay(500);
    }
    return false;
}

// ==================== 调试功能 ====================

void Ml307AtModem::debugLBSConfig()
{
    debugPrint("=== LBS配置状态检查 ===");

    String methodResponse = sendATWithResponse("AT+MLBSCFG=\"method\"");
    debugPrint("定位平台配置: " + methodResponse);

    String apiKeyResponse = sendATWithResponse("AT+MLBSCFG=\"apikey\"");
    debugPrint("API Key配置: " + apiKeyResponse);

    String signKeyResponse = sendATWithResponse("AT+MLBSCFG=\"signkey\"");
    debugPrint("签名密钥配置: " + signKeyResponse);

    String signEnResponse = sendATWithResponse("AT+MLBSCFG=\"signen\"");
    debugPrint("签名启用配置: " + signEnResponse);

    String pidResponse = sendATWithResponse("AT+MLBSCFG=\"pid\"");
    debugPrint("PID配置: " + pidResponse);

    String nearbtsResponse = sendATWithResponse("AT+MLBSCFG=\"nearbtsen\"");
    debugPrint("邻区信息配置: " + nearbtsResponse);

    String precisionResponse = sendATWithResponse("AT+MLBSCFG=\"precision\"");
    debugPrint("精度配置: " + precisionResponse);
}

// 智能退避计算 - 更温和的退避策略
unsigned long Ml307AtModem::calculateBackoffDelay() {
    if (_cmeErrorCount == 0) return 5000;  // 5秒
    if (_cmeErrorCount == 1) return 8000;  // 8秒
    if (_cmeErrorCount == 2) return 15000; // 15秒
    if (_cmeErrorCount == 3) return 30000; // 30秒
    return 60000; // 60秒，最大退避时间
}

// 检查是否应该跳过LBS请求
bool Ml307AtModem::shouldSkipLBSRequest() {
    if (_cmeErrorCount > 0) {
        unsigned long now = millis();
        unsigned long timeSinceLastError = now - _lastCMEErrorTime;
        if (timeSinceLastError < _backoffDelay) {
            // 减少日志频率，避免刷屏
            static unsigned long lastLogTime = 0;
            if (now - lastLogTime > 5000) { // 5秒打印一次
                debugPrint("CME错误退避中，剩余时间: " + String((_backoffDelay - timeSinceLastError) / 1000) + "秒");
                lastLogTime = now;
            }
            return true;
        }
    }
    return false;
}

// 处理CME错误 - 优化退避策略
void Ml307AtModem::handleCMEError() {
    _cmeErrorCount++;
    _lastCMEErrorTime = millis();
    _backoffDelay = calculateBackoffDelay();
    debugPrint("CME错误计数: " + String(_cmeErrorCount) + ", 退避时间: " + String(_backoffDelay / 1000) + "秒");
    
    // 如果连续错误次数过多，可以考虑重置模块
    if (_cmeErrorCount >= 5) {
        debugPrint("CME错误次数过多，建议检查网络状态或重启模块");
    }
}

// 重置CME错误计数
void Ml307AtModem::resetCMEErrorCount() {
    if (_cmeErrorCount > 0) {
        debugPrint("LBS请求成功，重置CME错误计数");
        _cmeErrorCount = 0;
        _backoffDelay = 5000;
    }
}