/*
 * ml307 AT Modem 驱动实现
 */

#include "Ml307AtModem.h"

#ifdef ENABLE_GSM
Ml307AtModem ml307(Serial1, GSM_RX_PIN, GSM_TX_PIN);
#else
Ml307AtModem ml307(Serial1, 16, 17);
#endif

Ml307AtModem::Ml307AtModem(HardwareSerial& serial, int rxPin, int txPin)
    : _serial(serial)
    , _rxPin(rxPin)
    , _txPin(txPin)
    , _gnssEnabled(false)
    , _lbsEnabled(false)
    , _debug(false)
{
}

bool Ml307AtModem::begin(uint32_t baudrate) {
    Serial.println("开始初始化ml307模块,波特率:" + String(baudrate));
    
    // 使用固定波特率初始化
    _serial.begin(baudrate, SERIAL_8N1, _rxPin, _txPin);
    delay(1000); // 增加延时，等待模块稳定
    
    // 清空缓冲区
    flushInput();
    
    while (!isReady()) {
        debugPrint("等待模块就绪...");
        delay(2000); // 增加等待时间
    }
    
    // 必须成功初始化
    while (!initModem()) {
        debugPrint("重试初始化模块...");
        delay(1000);
    }
    
    return true;
}

// 尝试特定波特率
bool Ml307AtModem::tryBaudrate(uint32_t baudrate) {
    _serial.begin(baudrate, SERIAL_8N1, _rxPin, _txPin);  // 指定引脚和串口配置
    delay(100);
    
    // 清空缓冲区
    while (_serial.available()) {
        _serial.read();
    }
    
    // 多次尝试AT测试
    for (int i = 0; i < 3; i++) {
        debugPrint("> AT");
        _serial.println("AT");
        
        // 等待响应
        String response = waitResponse(1000);
        debugPrint("< " + response);
        
        if (response.indexOf("OK") >= 0) {
            return true;
        }
        delay(100);
    }
    
    return false;
}

// 封装一个带重试的AT命令发送
bool Ml307AtModem::sendATWithRetry(const String& cmd, const String& expected, int maxRetry, uint32_t timeout) {
    for (int i = 0; i < maxRetry; ++i) {
        if (sendAT(cmd, expected, timeout)) {
            return true;
        }
        delay(500);
    }
    return false;
}

// 模块初始化配置
bool Ml307AtModem::initModem() {
    delay(2000);

    // 关闭回显
    if (!sendATWithRetry("ATE0")) {
        debugPrint("关闭回显失败");
        return false;
    }

    // 启用网络注册状态主动上报
    if (!sendATWithRetry("AT+CEREG=1")) {
        debugPrint("设置CEREG失败");
        return false;
    }
    

    // 检查 PIN 码状态
    int pinRetry = 0;
    const int maxPinRetry = 5;
    String pinStatus;
    do {
        pinStatus = sendATWithResponse("AT+CPIN?");
        if (pinStatus.indexOf("READY") >= 0) break;
        delay(500);
        pinRetry++;
    } while (pinRetry < maxPinRetry);
    if (pinStatus.indexOf("READY") < 0) {
        debugPrint("SIM卡 PIN 码未就绪");
        return false;
    }

    // 检查信号强度
    int csqRetry = 0;
    int csq = -1;
    do {
        csq = getCSQ();
        if (csq >= 5) break;
        delay(1000);
        csqRetry++;
    } while (csqRetry < 5);
    if (csq < 5) {
        debugPrint("信号太弱，无法注册网络");
        return false;
    }

    // 激活PDP上下文
    if (!sendATWithRetry("AT+MIPCALL?", "OK", 5, 5000)) {
        debugPrint("激活PDP上下文失败");
        return false;
    }

    // 等待网络注册
    while (!isNetworkReady()) {
        debugPrint("等待网络注册...");
        delay(2000);
    }

    return true;
}

void Ml307AtModem::setDebug(bool debug) {
    _debug = debug;
}

/*
1,0	未注册；ME 当前没有搜索要注册业务的新运营商
1,1	已注册，本地网
1,2	未注册，但 ME 正在搜索要注册业务的新运营商
1,3	注册被拒绝
1,4	未知(超出E-UTRAN网覆盖范围)
1,5	注册漫游网
*/
bool Ml307AtModem::isNetworkReady() {
    String pdpResponse = sendATWithResponse("AT+CEREG?");
    debugPrint("CEREG状态: " + pdpResponse);

    // 查找状态码
    int idx = pdpResponse.indexOf("+CEREG:");
    if (idx >= 0) {
        // 跳过 "+CEREG: " 找到状态值
        idx = pdpResponse.indexOf(',', idx);
        if (idx >= 0 && pdpResponse.length() > idx + 1) {
            char stat = pdpResponse.charAt(idx + 1);
            String statusMsg;
            switch (stat) {
                case '0': statusMsg = "未注册，未搜索新运营商"; break;
                case '1': statusMsg = "已注册，本地网"; break;
                case '2': statusMsg = "未注册，正在搜索新运营商"; break;
                case '3': statusMsg = "注册被拒绝"; break;
                case '4': statusMsg = "未知（超出覆盖范围）"; break;
                case '5': statusMsg = "已注册，漫游网"; break;
                default:  statusMsg = "未知状态"; break;
            }
            debugPrint("CEREG注册状态: " + String(stat) + " - " + statusMsg);
            return stat == '1' || stat == '5';
        }
    }
    
    debugPrint("CEREG响应格式异常");
    return false;
}

bool Ml307AtModem::reset() {
    return sendAT("AT+CFUN=1,1", "OK", 10000);
}

String Ml307AtModem::getLocalIP() {
    String response = sendATWithResponse("AT+CGPADDR=1");
    int start = response.indexOf("+CGPADDR: 1,\"");
    if (start >= 0) {
        start += 12;
        int end = response.indexOf("\"", start);
        if (end >= 0) {
            return response.substring(start, end);
        }
    }
    return "";
}

int Ml307AtModem::getCSQ() {
    String response = sendATWithResponse("AT+CSQ");
    int start = response.indexOf("+CSQ: ");
    if (start >= 0) {
        start += 6;
        return response.substring(start).toInt();
    }
    return -1;
}

String Ml307AtModem::getIMEI() {
    return sendATWithResponse("AT+CGSN");
}

String Ml307AtModem::getICCID() {
    String response = sendATWithResponse("AT+CCID");
    int start = response.indexOf("+CCID: ");
    if (start >= 0) {
        start += 7;
        return response.substring(start);
    }
    return "";
}

String Ml307AtModem::getModuleName() {
    return sendATWithResponse("AT+CGMM");
}

String Ml307AtModem::getCarrierName() {
    String response = sendATWithResponse("AT+COPS?");
    int start = response.indexOf("+COPS: 0,0,\"");
    if (start >= 0) {
        start += 11;
        int end = response.indexOf("\"", start);
        if (end >= 0) {
            return response.substring(start, end);
        }
    }
    return "";
}

bool Ml307AtModem::sendAT(const String& cmd, const String& expected, uint32_t timeout) {
    flushInput();
    
    debugPrint("> " + cmd);
    _serial.println(cmd);
    
    return expectResponse(expected, timeout);
}

String Ml307AtModem::sendATWithResponse(const String& cmd, uint32_t timeout) {
    flushInput();
    
    debugPrint("> " + cmd);
    _serial.println(cmd);
    
    return waitResponse(timeout);
}

bool Ml307AtModem::isReady() {
    // 发送AT命令并检查响应
    String response = sendATWithResponse("AT");
    
    // 检查多种可能的响应
    if (response.indexOf("OK") >= 0) {
        return true;
    }
    
    // 检查是否返回了模块信息（如ml307a等）
    if (response.indexOf("ml307") >= 0 || 
        response.indexOf("4G_DTU_TCP") >= 0 ||
        response.indexOf("PROD:") >= 0) {
        debugPrint("模块已就绪（返回模块信息）");
        return true;
    }
    
    return false;
}

void Ml307AtModem::flushInput() {
    while (_serial.available()) {
        _serial.read();
    }
}

String Ml307AtModem::waitResponse(uint32_t timeout) {
    String response;
    uint32_t start = millis();
    
    while (millis() - start < timeout) {
        if (_serial.available()) {
            char c = _serial.read();
            response += c;
            
            // 检查是否收到完整响应，增加更多可能的结束标记
            if (response.endsWith("\r\nOK\r\n") || 
                response.endsWith("\r\nERROR\r\n") ||
                response.endsWith("\r\n> ") ||
                response.endsWith("+MATREADY\r\n") ||  // 处理特殊响应
                response.endsWith("\"CONNECT\"\r\n") ||
                response.indexOf("ml307") >= 0 ||     // 检查模块信息
                response.indexOf("4G_DTU_TCP") >= 0) { // 检查产品信息
                break;
            }
        }
    }
    
    if (_debug) {
        if (response.length() == 0) {
            debugPrint("AT响应超时");
        } else {
            debugPrint("AT响应: " + response);
        }
    }
    
    return response;
}

bool Ml307AtModem::expectResponse(const String& expected, uint32_t timeout) {
    String response = waitResponse(timeout);
    return response.indexOf(expected) >= 0;
}

void Ml307AtModem::debugPrint(const String& msg) {
    if (_debug) {
        Serial.println("[ml307atmodem] [debug] " + msg);
    }
}

bool Ml307AtModem::gnssPower(bool on) {
    String cmd = "AT+CGNSPWR=";
    cmd += (on ? "1" : "0");
    return sendAT(cmd);
}

String Ml307AtModem::getGNSSInfo() {
    return sendATWithResponse("AT+CGNSINF");
}

// GNSS定位功能实现
bool Ml307AtModem::gnssInit() {
    debugPrint("初始化GNSS模块...");
    
    // 检查GNSS是否支持
    if (!sendATWithRetry("AT+CGNSPWR?", "OK", 3, 2000)) {
        debugPrint("GNSS模块不支持");
        return false;
    }
    
    // 开启GNSS电源
    if (!gnssPower(true)) {
        debugPrint("开启GNSS电源失败");
        return false;
    }
    
    // 等待GNSS模块启动
    delay(3000);
    
    // 检查GNSS状态
    int retry = 0;
    while (retry < 10) {
        if (isGNSSReady()) {
            debugPrint("GNSS模块初始化成功");
            _gnssEnabled = true;
            return true;
        }
        delay(1000);
        retry++;
    }
    
    debugPrint("GNSS模块初始化失败");
    return false;
}

bool Ml307AtModem::isGNSSReady() {
    String response = sendATWithResponse("AT+CGNSINF");
    return response.indexOf("+CGNSINF: ") >= 0 && response.indexOf(",1,") >= 0;
}

gps_data_t Ml307AtModem::getGNSSData() {
    if (millis() - _lastGNSSUpdate > 1000) { // 每秒更新一次
        updateGNSSData();
    }
    return _gnssData;
}

bool Ml307AtModem::updateGNSSData() {
    if (!_gnssEnabled) {
        return false;
    }
    
    String response = sendATWithResponse("AT+CGNSINF");
    if (parseGNSSInfo(response)) {
        _lastGNSSUpdate = millis();
        return true;
    }
    return false;
}

bool Ml307AtModem::parseGNSSInfo(const String& response) {
    // 重置数据
    resetGNSSData();
    
    // 查找CGNSINF响应
    int start = response.indexOf("+CGNSINF: ");
    if (start < 0) {
        return false;
    }
    
    // 跳过"+CGNSINF: "
    start += 10;
    int end = response.indexOf("\r\n", start);
    if (end < 0) {
        end = response.length();
    }
    
    String data = response.substring(start, end);
    
    // 解析CGNSINF数据格式：
    // +CGNSINF: <GNSS run status>,<fix status>,<UTC date & Time>,<latitude>,<longitude>,<MSL altitude>,<speed over ground>,<course over ground>,<fix mode>,<reserved1>,<HDOP>,<PDOP>,<VDOP>,<reserved2>,<GNSS satellites in View>,<GNSS satellites used>,<reserved3>,<c/n0 max>,<HPA>,<VPA>
    
    int commaCount = 0;
    int lastComma = -1;
    String values[20];
    int valueIndex = 0;
    
    for (int i = 0; i < data.length() && valueIndex < 20; i++) {
        if (data.charAt(i) == ',') {
            if (lastComma >= 0) {
                values[valueIndex++] = data.substring(lastComma + 1, i);
            } else {
                values[valueIndex++] = data.substring(0, i);
            }
            lastComma = i;
            commaCount++;
        }
    }
    
    // 添加最后一个值
    if (lastComma >= 0 && valueIndex < 20) {
        values[valueIndex++] = data.substring(lastComma + 1);
    }
    
    // 检查定位状态
    if (valueIndex < 2 || values[1] != "1") {
        return false; // 未定位
    }
    
    // 解析时间 (格式: yyyyMMddHHmmss.sss)
    if (valueIndex > 2 && values[2].length() >= 14) {
        String timeStr = values[2];
        _gnssData.year = timeStr.substring(0, 4).toInt();
        _gnssData.month = timeStr.substring(4, 6).toInt();
        _gnssData.day = timeStr.substring(6, 8).toInt();
        _gnssData.hour = timeStr.substring(8, 10).toInt();
        _gnssData.minute = timeStr.substring(10, 12).toInt();
        _gnssData.second = timeStr.substring(12, 14).toInt();
    }
    
    // 解析纬度
    if (valueIndex > 3 && values[3].length() > 0) {
        _gnssData.latitude = values[3].toDouble();
    }
    
    // 解析经度
    if (valueIndex > 4 && values[4].length() > 0) {
        _gnssData.longitude = values[4].toDouble();
    }
    
    // 解析海拔高度
    if (valueIndex > 5 && values[5].length() > 0) {
        _gnssData.altitude = values[5].toDouble();
    }
    
    // 解析速度 (m/s转换为km/h)
    if (valueIndex > 6 && values[6].length() > 0) {
        _gnssData.speed = values[6].toDouble() * 3.6;
    }
    
    // 解析航向角
    if (valueIndex > 7 && values[7].length() > 0) {
        _gnssData.heading = values[7].toDouble();
    }
    
    // 解析HDOP
    if (valueIndex > 10 && values[10].length() > 0) {
        _gnssData.hdop = values[10].toDouble();
    }
    
    // 解析可见卫星数量
    if (valueIndex > 14 && values[14].length() > 0) {
        _gnssData.satellites = values[14].toInt();
    }
    
    debugPrint("GNSS数据更新成功: " + String(_gnssData.latitude, 6) + ", " + String(_gnssData.longitude, 6));
    return true;
}

void Ml307AtModem::resetGNSSData() {
    memset(&_gnssData, 0, sizeof(gps_data_t));
}

bool Ml307AtModem::enableLBS(bool enable) {
    debugPrint("配置LBS: " + String(enable ? "启用" : "禁用"));
    if (enable) {
        // 配置为OneOs定位服务
        if (!sendATWithRetry("AT+MLBSCFG=\"method\",40", "OK", 3, 2000)) {
            debugPrint("配置LBS方法失败");
            return false;
        }
        
        // 配置使用邻区信息提高精度
        if (!sendATWithRetry("AT+MLBSCFG=\"nearbtsen\",1", "OK", 3, 2000)) {
            debugPrint("配置邻区信息失败");
            return false;
        }

        // 配置精度为8位
        if (!sendATWithRetry("AT+MLBSCFG=\"precision\",8", "OK", 3, 2000)) {
            debugPrint("配置精度失败");
            return false;
        }

        _lbsEnabled = true;
        _lastLBSUpdate = 0;  // 重置更新时间
        return true;
    } else {
        _lbsEnabled = false;
        return true;
    }
}

bool Ml307AtModem::isLBSEnabled() {
    String response = sendATWithResponse("AT+MLBSCFG=\"method\"");
    // 检查是否配置了定位平台
    return response.indexOf("+MLBSCFG: \"method\",") >= 0;
}

lbs_data_t Ml307AtModem::getLBSData() {
    // 每秒最多更新一次
    if (millis() - _lastLBSUpdate > 1000) {
        updateLBSData();
    }
    return _lbsData;
}

bool Ml307AtModem::updateLBSData() {
    if (!_lbsEnabled) {
        return false;
    }
    
    // 控制更新频率
    if (_lastLBSUpdate > 0 && (millis() - _lastLBSUpdate) < 1000) {
        return false;
    }
    
    String response = sendATWithResponse("AT+MLBSLOC", 5000);
    if (parseLBSInfo(response)) {
        _lastLBSUpdate = millis();
        return true;
    }
    
    return false;
}

bool Ml307AtModem::enableGNSS(bool enable) {
    debugPrint("配置GNSS: " + String(enable ? "启用" : "禁用"));
    
    // 先检查GNSS是否支持
    if (!sendATWithRetry("AT+CGNSPWR?", "OK", 3, 2000)) {
        debugPrint("GNSS模块不支持");
        return false;
    }
    
    // 控制GNSS电源
    if (enable) {
        // 开启GNSS电源
        if (!gnssPower(true)) {
            debugPrint("开启GNSS电源失败");
            return false;
        }
        // 初始化GNSS
        return gnssInit();
    } else {
        // 关闭GNSS电源
        return gnssPower(false);
    }
}

bool Ml307AtModem::isGNSSEnabled() {
    String response = sendATWithResponse("AT+CGNSPWR?");
    return response.indexOf("+CGNSPWR: 1") >= 0;
}

bool Ml307AtModem::parseLBSInfo(const String& response) {
    // 重置数据有效性
    resetLBSData();
    
    if (response.indexOf("+MLBSLOC:") == -1) {
        if (response.indexOf("+CME ERROR:") != -1) {
            int errorStart = response.indexOf("+CME ERROR:") + 11;
            String errorCode = response.substring(errorStart);
            errorCode.trim();
            _lbsData.stat = errorCode.toInt();
            debugPrint("LBS错误: " + errorCode);
        } else {
            debugPrint("LBS响应异常，未找到+MLBSLOC");
        }
        return false;
    }

    // 解析MLBSLOC数据, 格式: +MLBSLOC: <stat>,<longitude>,<latitude>[,<radius>][,<descr>]
    int startPos = response.indexOf("+MLBSLOC:") + 9;
    String dataStr = response.substring(startPos);
    dataStr.trim();
    
    int commaPos = 0;
    int lastCommaPos = -1;
    int valueIndex = 0;
    
    while(valueIndex <= 4) { // 最多解析5个字段
        commaPos = dataStr.indexOf(',', lastCommaPos + 1);

        String value;
        if (commaPos != -1) {
            value = dataStr.substring(lastCommaPos + 1, commaPos);
        } else {
            value = dataStr.substring(lastCommaPos + 1);
        }
        value.trim();

        if (value.startsWith("\"")) {
            value = value.substring(1, value.length() - 1);
        }

        switch (valueIndex) {
            case 0: _lbsData.stat = value.toInt(); break;
            case 1: _lbsData.longitude = value.toFloat(); break;
            case 2: _lbsData.latitude = value.toFloat(); break;
            case 3: _lbsData.radius = value.toInt(); break;
            case 4: _lbsData.descr = value; break;
        }

        if (commaPos == -1) {
            break;
        }
        
        lastCommaPos = commaPos;
        valueIndex++;
    }

    if (_lbsData.stat == 100) {
        _lbsData.valid = true;
        debugPrint("LBS解析成功: " + 
                  String(_lbsData.longitude, 6) + ", " + 
                  String(_lbsData.latitude, 6) + 
                  (_lbsData.radius > 0 ? ", r:" + String(_lbsData.radius) : "") +
                  (!_lbsData.descr.isEmpty() ? ", d:" + _lbsData.descr : ""));
    } else {
        debugPrint("LBS状态码错误: " + String(_lbsData.stat));
    }

    return _lbsData.valid;
}

void Ml307AtModem::resetLBSData() {
    memset(&_lbsData, 0, sizeof(lbs_data_t));
    _lbsData.valid = false;
}

// 在实现文件中添加线程安全的AT命令方法
bool Ml307AtModem::sendATThreadSafe(const String& cmd, const String& expected, uint32_t timeout) {
    std::lock_guard<std::mutex> lock(_atMutex);
    return sendAT(cmd, expected, timeout);
}

String Ml307AtModem::sendATWithResponseThreadSafe(const String& cmd, uint32_t timeout) {
    std::lock_guard<std::mutex> lock(_atMutex);
    return sendATWithResponse(cmd, timeout);
}
