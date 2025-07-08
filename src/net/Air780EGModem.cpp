/*
 * Air780EG 4G+GNSS Modem 驱动实现
 */

#include "Air780EGModem.h"
#include <HardwareSerial.h>

#ifdef USE_AIR780EG_GSM

// 全局实例声明
Air780EGModem air780eg_modem(Serial2, GSM_RX_PIN, GSM_TX_PIN, GSM_EN);

// GNSS和LBS数据解析用的临时缓冲区
char g_gnssFields[21][32];
char g_lbsParts[5][32];

Air780EGModem::Air780EGModem(HardwareSerial& serial, int rxPin, int txPin, int enPin)
    : _serial(serial), _rxPin(rxPin), _txPin(txPin), _enPin(enPin),
      _lbsEnabled(false), _gnssEnabled(false),
      _isNetworkReady(false), _lastNetworkReadyCheckTime(0),
      _initState(INIT_IDLE), _initStartTime(0), _lastInitCheck(0),
      _lastLBSUpdate(0), _isLBSLoading(false),
      _lastCMEErrorTime(0), _cmeErrorCount(0), _backoffDelay(0),
      _mqttMessageHandler(nullptr) {
    // 初始化LBS数据
    _lbsData.valid = false;
    _lbsData.latitude = 0.0;
    _lbsData.longitude = 0.0;
    _lbsData.radius = 0;
    _lbsData.stat = 0;
    _lbsData.state = LBSState::IDLE;
    _lbsData.lastRequestTime = 0;
    _lbsData.requestStartTime = 0;
    _lbsData.timestamp = 0;
}

bool Air780EGModem::begin(uint32_t baudrate) {
    // 配置使能引脚
    if (_enPin >= 0) {
        pinMode(_enPin, OUTPUT);
        digitalWrite(_enPin, LOW);
        delay(500);
        digitalWrite(_enPin, HIGH);
        delay(2000);
        
        if (!digitalRead(_enPin)) {
            return false;
        }
    }
    
    // 初始化串口
    if (!tryBaudrate(baudrate)) {
        uint32_t baudrates[] = {115200, 9600, 38400, 57600};
        for (int i = 0; i < 4; i++) {
            if (baudrates[i] != baudrate && tryBaudrate(baudrates[i])) {
                break;
            }
        }
    }
    
    return initModem();
}

bool Air780EGModem::tryBaudrate(uint32_t baudrate) {
    _serial.begin(baudrate, SERIAL_8N1, _rxPin, _txPin);
    delay(100);
    flushInput();
    
    for (int i = 0; i < 3; i++) {
        if (sendAT("AT", "OK", 1000)) {
            return true;
        }
        delay(500);
    }
    return false;
}

bool Air780EGModem::initModem() {
    // 基础AT命令测试
    if (!sendATWithRetry("AT", "OK", 5, 2000)) {
        return false;
    }
    
    // 关闭回显
    sendAT("ATE0");
    
    // 检查SIM卡
    if (!sendATWithRetry("AT+CPIN?", "READY", 3, 3000)) {
        return false;
    }
    
    // 设置网络注册检查
    sendAT("AT+CREG=1");
    sendAT("AT+CGREG=1");
    
    // 等待网络注册
    for (int i = 0; i < 30; i++) {
        if (isNetworkReadyCheck()) {
            break;
        }
        delay(2000);
    }
    
    // 启动后台初始化状态机
    _initState = INIT_IDLE;
    _initStartTime = millis();
    _lastInitCheck = 0;

    enableGNSSAutoReport(false);
    return true;
}

bool Air780EGModem::powerOn() {
    if (_enPin >= 0) {
        digitalWrite(_enPin, HIGH);
        delay(1000);
        return true;
    }
    return false;
}

bool Air780EGModem::powerOff() {
    if (_enPin >= 0) {
        digitalWrite(_enPin, LOW);
        delay(1000);
        return true;
    }
    return false;
}

bool Air780EGModem::reset() {
    debugPrint("Air780EG: 重置模块");
    
    if (_enPin >= 0) {
        digitalWrite(_enPin, LOW);
        delay(1000);
        digitalWrite(_enPin, HIGH);
        delay(3000);
    } else {
        sendAT("AT+CFUN=1,1");  // 软重启
        delay(5000);
    }
    
    return initModem();
}

void Air780EGModem::setDebug(bool debug) {
    _debug = debug;
}

void Air780EGModem::setMQTTMessageHandler(void (*callback)(String topic, String payload)) {
    _mqttMessageHandler = callback;
    debugPrint("Air780EG: MQTT消息处理器已注册");
}

void Air780EGModem::debugPrint(const String& msg) {
    if (_debug) {
        Serial.println("[Air780EG] " + msg);
    }
}

bool Air780EGModem::isNetworkReadyCheck() {
    if (millis() - _lastNetworkReadyCheckTime < 5000) {
        return _isNetworkReady;
    }
    
    _lastNetworkReadyCheckTime = millis();
    _isNetworkReady = true;
    return true;
}

String Air780EGModem::getLocalIP() {
    String response = sendATWithResponse("AT+CGPADDR=1", 3000);
    int start = response.indexOf("\"") + 1;
    int end = response.indexOf("\"", start);
    if (start > 0 && end > start) {
        return response.substring(start, end);
    }
    return "";
}

int Air780EGModem::getCSQ() {
    String response = sendATWithResponse("AT+CSQ", 2000);
    int start = response.indexOf("+CSQ: ") + 6;
    int end = response.indexOf(",", start);
    if (start > 5 && end > start) {
        return response.substring(start, end).toInt();
    }
    return -1;
}

String Air780EGModem::getIMEI() {
    String response = sendATWithResponse("AT+CGSN", 2000);
    response.trim();
    for (int i = 0; i < response.length(); i++) {
        if (isdigit(response.charAt(i))) {
            String imei = response.substring(i);
            if (imei.length() >= 15) {
                return imei.substring(0, 15);
            }
        }
    }
    return "";
}

String Air780EGModem::getICCID() {
    String response = sendATWithResponse("AT+CCID", 2000);
    int start = response.indexOf("+CCID: ") + 7;
    if (start > 6) {
        String iccid = response.substring(start);
        iccid.trim();
        return iccid;
    }
    return "";
}

String Air780EGModem::getModuleName() {
    String response = sendATWithResponse("ATI", 2000);
    if (response.indexOf("Air780EG") >= 0) {
        return "Air780EG";
    }
    return response;
}

String Air780EGModem::getCarrierName() {
    String response = sendATWithResponse("AT+COPS?", 3000);
    int start = response.indexOf("\"") + 1;
    int end = response.indexOf("\"", start);
    if (start > 0 && end > start) {
        return response.substring(start, end);
    }
    return "";
}

String Air780EGModem::getNetworkType() {
    String response = sendATWithResponse("AT+COPS?", 3000);
    if (response.indexOf(",7") >= 0) return "LTE";
    if (response.indexOf(",2") >= 0) return "3G";
    if (response.indexOf(",0") >= 0) return "2G";
    return "Unknown";
}

// GNSS功能实现
bool Air780EGModem::enableGNSS(bool enable) {
    if (enable) {
        Serial.println("[Air780EG] 启用GNSS");
        
        // 1. 开启GNSS电源
        Serial.println("[Air780EG] 发送GNSS开启命令...");
        if (sendAT("AT+CGNSPWR=1", "OK", 5000)) {
            Serial.println("[Air780EG] GNSS电源开启成功");
            _gnssEnabled = true;
            delay(2000); // 等待GNSS启动
            
            // 2. 启用位置辅助定位
            Serial.println("[Air780EG] 启用位置辅助定位...");
            sendAT("AT+CGNSAID=31,1,1,1", "OK", 3000);
            
            // 3. 设置定位信息自动上报
            Serial.println("[Air780EG] 设置定位信息自动上报...");
            sendAT("AT+CGNSURC=1", "OK", 3000);
            
            Serial.println("[Air780EG] GNSS初始化完成");
            return true;
        } else {
            Serial.println("[Air780EG] GNSS电源开启失败");
            return false;
        }
    } else {
        Serial.println("[Air780EG] 禁用GNSS");
        
        // 1. 首先关闭GNSS自动上报
        Serial.println("[Air780EG] 关闭GNSS自动上报...");
        sendAT("AT+CGNSURC=0", "OK", 3000);
        
        // 2. 然后关闭GNSS电源
        if (sendAT("AT+CGNSPWR=0", "OK", 3000)) {
            Serial.println("[Air780EG] GNSS已禁用");
            _gnssEnabled = false;
            return true;
        } else {
            Serial.println("[Air780EG] GNSS禁用失败");
            return false;
        }
    }
}

// 控制GNSS自动上报
bool Air780EGModem::enableGNSSAutoReport(bool enable) {
    if (enable) {
        Serial.println("[Air780EG] 启用GNSS自动上报");
        if (sendAT("AT+CGNSURC=1", "OK", 3000)) {
            Serial.println("[Air780EG] GNSS自动上报已启用");
            return true;
        } else {
            Serial.println("[Air780EG] GNSS自动上报启用失败");
            return false;
        }
    } else {
        Serial.println("[Air780EG] 禁用GNSS自动上报");
        if (sendAT("AT+CGNSURC=0", "OK", 3000)) {
            Serial.println("[Air780EG] GNSS自动上报已禁用");
            return true;
        } else {
            Serial.println("[Air780EG] GNSS自动上报禁用失败");
            return false;
        }
    }
}

bool Air780EGModem::isGNSSEnabled() {
    String response = sendATWithResponse("AT+CGNSPWR?", 2000);
    return response.indexOf("+CGNSPWR: 1") >= 0;
}

String Air780EGModem::getGNSSInfo() {
    String response = sendATWithResponse("AT+CGNSINF", 3000);
    if (response.length() > 0) {
        // 解析并格式化GNSS信息
        int commaCount = 0;
        int lastPos = 0;
        static String fields[22];
        
        for (int i = 0; i < response.length() && commaCount < 21; i++) {
            if (response.charAt(i) == ',') {
                fields[commaCount] = response.substring(lastPos, i);
                lastPos = i + 1;
                commaCount++;
            }
        }
        if (commaCount < 21) {
            fields[commaCount] = response.substring(lastPos);
        }
        
        if (commaCount >= 15) {
            String info = "GNSS状态: ";
            info += (fields[0] == "1") ? "开启" : "关闭";
            info += ", 定位: ";
            info += (fields[1] == "1") ? "成功" : "失败";
            info += ", 可见卫星: " + fields[14];
            info += ", 使用卫星: " + fields[15];
            
            if (fields[1] == "1" && fields[4].length() > 0 && fields[5].length() > 0) {
                info += ", 位置: " + fields[4] + "," + fields[5];
            }
            
            return info;
        }
    }
    return "GNSS信息获取失败";
}

// 检查GNSS是否已定位
bool Air780EGModem::isGNSSFixed() {
    Serial.println("[Air780EG] 检查GNSS是否已定位");
    String response = sendATWithResponse("AT+CGNSINF", 2000);
    if (response.length() > 0) {
        int firstComma = response.indexOf(',');
        int secondComma = response.indexOf(',', firstComma + 1);
        if (firstComma > 0 && secondComma > 0) {
            String fixStatus = response.substring(firstComma + 1, secondComma);
            return fixStatus == "1";
        }
    }
    return false;
}

// 简化版本：移除频率控制，由GPSDataCache统一管理
bool Air780EGModem::updateGNSSData() {
    if (!_gnssEnabled) return false;
    
    String response = sendATWithResponse("AT+CGNSINF", 3000);
    if (response.length() > 0) {
        _lastGNSSRaw = response;
        return parseGNSSResponse(response);
    }
    
    return false;
}

bool Air780EGModem::parseGNSSResponse(const String& response) {
    GNSS_DEBUG("Enter parseGNSSResponse, response len: %d", response.length());
    int start = response.indexOf("+CGNSINF: ") + 10;
    if (start < 10) {
        DEBUG_WARN("Air780EG", "GNSS响应格式错误: %s", response.c_str());
        return false;
    }
    String data = response.substring(start);
    data.trim();
    GNSS_DEBUG("parseGNSSResponse: data len: %d, data: %s", data.length(), data.c_str());
    // 分割数据到 char 数组
    int commaCount = 0, lastIndex = 0, fieldLen = 0;
    memset(g_gnssFields, 0, sizeof(g_gnssFields));
    for (int i = 0; i < data.length() && commaCount < 20; i++) {
        if (data.charAt(i) == ',') {
            fieldLen = i - lastIndex;
            strncpy(g_gnssFields[commaCount], data.c_str() + lastIndex, std::min(fieldLen, 31));
            g_gnssFields[commaCount][std::min(fieldLen, 31)] = '\0';
            lastIndex = i + 1;
            commaCount++;
        }
    }
    if (commaCount < 20) {
        fieldLen = data.length() - lastIndex;
        strncpy(g_gnssFields[commaCount], data.c_str() + lastIndex, std::min(fieldLen, 31));
        g_gnssFields[commaCount][std::min(fieldLen, 31)] = '\0';
    }
    GNSS_DEBUG("parseGNSSResponse: commaCount=%d", commaCount);
    
    // 检查字段数量
    if (commaCount < 15) {
        DEBUG_WARN("Air780EG", "GNSS字段数量不足: %d", commaCount + 1);
        return false;
    }
    
    // 解析字段
    bool gnssRunning = g_gnssFields[0][0] == '1';
    bool fixStatus = g_gnssFields[1][0] == '1';
    
    Serial.println("[Air780EG] GNSS运行状态: " + String(gnssRunning ? "开启" : "关闭"));
    Serial.println("[Air780EG] 定位状态: " + String(fixStatus ? "已定位" : "未定位"));
    
    // 即使未定位也要解析数据，只是标记为无效
    _gnssData.valid = gnssRunning && fixStatus;
    _gnssData.timestamp = String(g_gnssFields[2]);
    
    // 只有在定位成功时才解析位置信息
    if (fixStatus && g_gnssFields[3][0] != '\0' && g_gnssFields[4][0] != '\0') {
        _gnssData.latitude = atof(g_gnssFields[3]);
        _gnssData.longitude = atof(g_gnssFields[4]);
        _gnssData.altitude = atof(g_gnssFields[5]);
        _gnssData.speed = atof(g_gnssFields[6]);
        _gnssData.course = atof(g_gnssFields[7]);
        _gnssData.fix_mode = String(g_gnssFields[8]);
        _gnssData.hdop = atof(g_gnssFields[10]);
        _gnssData.satellites = atoi(g_gnssFields[15]);
        
        Serial.printf("[Air780EG] GNSS数据: Lat=%.6f, Lon=%.6f, Sats=%d\n",
                          _gnssData.latitude, _gnssData.longitude, _gnssData.satellites);
    } else {
        // 未定位时清空位置数据，但保留其他信息
        _gnssData.latitude = 0.0;
        _gnssData.longitude = 0.0;
        _gnssData.altitude = 0.0;
        _gnssData.speed = 0.0;
        _gnssData.course = 0.0;
        _gnssData.fix_mode = String(g_gnssFields[8]);
        _gnssData.hdop = atof(g_gnssFields[10]);
        _gnssData.satellites = atoi(g_gnssFields[15]);
        
        Serial.println("[Air780EG] GNSS未定位，等待卫星信号...");
        Serial.println("[Air780EG] 可见卫星: " + String(_gnssData.satellites));
        Serial.println("[Air780EG] HDOP: " + String(_gnssData.hdop));
    }
    
    // 解析成功，无论是否定位
    return true;
}

GNSSData Air780EGModem::getGNSSData() {
    return _gnssData;
}

bool Air780EGModem::isGNSSDataValid() {
    return _gnssData.valid && (millis() - _lastGNSSUpdate < 10000);
}

bool Air780EGModem::setGNSSUpdateRate(int hz) {
    if (hz < 1 || hz > 10) return false;
    
    String cmd;
    switch (hz) {
        case 1:  cmd = "AT+CGNSSEQ=\"RMC\",1000"; break;
        case 2:  cmd = "AT+CGNSSEQ=\"RMC\",500"; break;
        case 5:  cmd = "AT+CGNSSEQ=\"RMC\",200"; break;
        case 10: cmd = "AT+CGNSSEQ=\"RMC\",100"; break;
        default: return false;
    }
    
    if (sendAT(cmd, "OK", 2000)) {
        // 不再保存频率值，由GPSDataCache统一管理
        return true;
    }
    return false;
}

String Air780EGModem::getGNSSRawData() {
    return _lastGNSSRaw;
}

// GPS数据转换 (兼容现有接口)
gps_data_t Air780EGModem::getGPSData() {
    gps_data_t gpsData;
    
    // 初始化GPS数据结构
    reset_gps_data(gpsData);
    
    if (_gnssData.valid) {
        gpsData.latitude = _gnssData.latitude;
        gpsData.longitude = _gnssData.longitude;
        gpsData.altitude = _gnssData.altitude;
        gpsData.speed = _gnssData.speed;
        gpsData.heading = _gnssData.course;  // course -> heading
        gpsData.satellites = _gnssData.satellites;
        gpsData.hdop = _gnssData.hdop;
        
        // 解析时间戳 (假设格式为 YYYYMMDDHHMMSS.sss)
        if (_gnssData.timestamp.length() >= 14) {
            String ts = _gnssData.timestamp;
            gpsData.year = ts.substring(0, 4).toInt();
            gpsData.month = ts.substring(4, 6).toInt();
            gpsData.day = ts.substring(6, 8).toInt();
            gpsData.hour = ts.substring(8, 10).toInt();
            gpsData.minute = ts.substring(10, 12).toInt();
            gpsData.second = ts.substring(12, 14).toInt();
            if (ts.length() > 15) {
                gpsData.centisecond = ts.substring(15, 17).toInt();
            }
        }
        
        gpsData.gpsHz = 1; // 默认1Hz，实际频率由GPSDataCache管理
    }
    
    return gpsData;
}

bool Air780EGModem::isGPSDataValid() {
    return isGNSSDataValid();
}

// LBS功能实现 (参考ml307实现)
bool Air780EGModem::enableLBS(bool enable) {
    _lbsEnabled = enable;
    if (enable) {
        debugPrint("[Air780EG] 启用LBS定位");
        // Air780EG的LBS配置可能与ml307不同，需要查阅具体AT指令
        return sendAT("AT+CLBS=1", "OK", 3000);
    } else {
        debugPrint("[Air780EG] 禁用LBS定位");
        return sendAT("AT+CLBS=0", "OK", 3000);
    }
}

bool Air780EGModem::isLBSEnabled() {
    return _lbsEnabled;
}

String Air780EGModem::getLBSRawData() {
    return _lastLBSLocation;
}

bool Air780EGModem::updateLBSData() {
    if (!_lbsEnabled) return false;
    // 避免过于频繁的LBS请求
    if (millis() - _lastLBSUpdate < 30000) {
        debugPrint("[Air780EG] LBS请求过于频繁，跳过");
        return !_lastLBSLocation.isEmpty();
    }
    if (shouldSkipLBSRequest()) {
        debugPrint("[Air780EG] LBS退避中，跳过");
        return false;
    }
    // 1. 检查网络注册和信号强度
    String creg = sendATWithResponse("AT+CREG?", 2000);
    if (creg.indexOf(",1") < 0 && creg.indexOf(",5") < 0) {
        Serial.println("[Air780EG] ❌ 网络未注册，无法LBS定位: " + creg);
        return false;
    }
    int csq = getCSQ();
    if (csq < 10) {
        Serial.printf("[Air780EG] ❌ 信号弱(%d)，无法LBS定位\n", csq);
        return false;
    }
    _isLBSLoading = true;
    _lastLBSUpdate = millis();
    debugPrint("[Air780EG] 开始LBS定位请求...");
    // 2. 设置LBS服务器
    if (!setupLBSServer()) {
        _isLBSLoading = false;
        debugPrint("[Air780EG] LBS服务器配置失败");
        return false;
    }
    // 3. 设置并激活PDP上下文
    if (!setupPDPContext()) {
        _isLBSLoading = false;
        debugPrint("[Air780EG] PDP上下文设置失败");
        return false;
    }
    // 4. 执行LBS定位查询
    Serial.println("[Air780EG] 发送LBS定位指令: AT+CIPGSMLOC=1,1");
    String response = sendATWithResponse("AT+CIPGSMLOC=1,1", 35000); // 35秒超时
    Serial.println("[Air780EG] LBS原始响应: " + response);
    // 5. 去激活PDP上下文
    sendAT("AT+SAPBR=0,1", "OK", 5000);
    _isLBSLoading = false;
    if (response.length() > 0) {
        _lastLBSLocation = response;
        bool parseResult = parseLBSResponse(response);
        Serial.printf("[Air780EG] LBS响应解析结果: %s\n", parseResult ? "成功" : "失败");
        if (!parseResult) {
            Serial.println("[Air780EG] LBS响应解析失败，原始数据: " + response);
        }
        return parseResult;
    }
    debugPrint("[Air780EG] LBS请求无响应");
    return false;
}

bool Air780EGModem::setupLBSServer() {
    // 设置LBS服务器域名和端口
    if (!sendAT("AT+GSMLOCFG=\"bs.openluat.com\",12411", "OK", 5000)) {
        debugPrint("[Air780EG] LBS服务器配置失败");
        return false;
    }
    debugPrint("[Air780EG] LBS服务器配置成功");
    return true;
}

bool Air780EGModem::setupPDPContext() {
    // 设置承载类型为GPRS
    if (!sendAT("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "OK", 5000)) {
        debugPrint("[Air780EG] 设置承载类型失败");
        return false;
    }
    
    // 设置APN参数（空值表示自动获取）
    if (!sendAT("AT+SAPBR=3,1,\"APN\",\"\"", "OK", 5000)) {
        debugPrint("[Air780EG] 设置APN失败");
        return false;
    }
    
    // 激活GPRS PDP上下文
    if (!sendAT("AT+SAPBR=1,1", "OK", 10000)) {
        debugPrint("[Air780EG] 激活PDP上下文失败");
        return false;
    }
    
    // 查询激活状态
    String response = sendATWithResponse("AT+SAPBR=2,1", 5000);
    if (response.indexOf("+SAPBR: 1,1,") >= 0) {
        debugPrint("[Air780EG] PDP上下文激活成功");
        return true;
    } else {
        debugPrint("[Air780EG] PDP上下文激活验证失败");
        return false;
    }
}

bool Air780EGModem::parseLBSResponse(const String& response) {
    debugPrint("[Air780EG] 解析LBS响应: " + response);
    // 查找 +CIPGSMLOC: 响应
    int startPos = response.indexOf("+CIPGSMLOC:");
    if (startPos < 0) {
        debugPrint("[Air780EG] 未找到CIPGSMLOC响应");
        Serial.println("[Air780EG] LBS响应无CIPGSMLOC字段，原始: " + response);
        return false;
    }
    // 提取响应数据部分
    int lineEnd = response.indexOf('\n', startPos);
    if (lineEnd < 0) lineEnd = response.length();
    String dataLine = response.substring(startPos, lineEnd);
    dataLine.trim();
    debugPrint("[Air780EG] 提取的数据行: " + dataLine);
    // 解析格式: +CIPGSMLOC: 0,034.7983328,114.3214505,2023/06/05,14:38:50
    int colonPos = dataLine.indexOf(':');
    if (colonPos < 0) return false;
    String data = dataLine.substring(colonPos + 1);
    data.trim();
    // 按逗号分割数据
    int commaCount = 0;
    int lastComma = -1;
    for (int i = 0; i < 5; ++i) memset(g_lbsParts[i], 0, sizeof(g_lbsParts[i]));
    for (int i = 0; i < data.length() && commaCount < 5; i++) {
        if (data.charAt(i) == ',') {
            strncpy(g_lbsParts[commaCount], data.c_str() + lastComma + 1, std::min(i - lastComma - 1, 31));
            g_lbsParts[commaCount][std::min(i - lastComma - 1, 31)] = '\0';
            lastComma = i;
            commaCount++;
        }
    }
    if (commaCount == 4) {
        int len = (int)data.length() - (int)lastComma - 1;
        int copyLen = len < 31 ? (len > 0 ? len : 0) : 31;
        strncpy(g_lbsParts[4], data.c_str() + lastComma + 1, copyLen);
        g_lbsParts[4][copyLen] = '\0';
    }
    if (commaCount < 4) {
        debugPrint("[Air780EG] LBS数据格式错误，字段数量不足");
        Serial.println("[Air780EG] LBS数据字段不足，原始: " + data);
        return false;
    }
    // 解析各个字段
    int errorCode = atoi(g_lbsParts[0]);
    if (errorCode != 0) {
        Serial.printf("[Air780EG] LBS定位失败，错误码: %d\n", errorCode);
        Serial.println("[Air780EG] LBS失败原始数据: " + data);
        return false;
    }
    // 解析经纬度
    float longitude = atof(g_lbsParts[1]);
    float latitude = atof(g_lbsParts[2]);
    if (longitude == 0.0 && latitude == 0.0) {
        debugPrint("[Air780EG] LBS定位数据无效（0,0）");
        Serial.println("[Air780EG] LBS返回经纬度为0,0，原始: " + data);
        return false;
    }
    // 更新LBS数据缓存
    _lbsData.latitude = latitude;
    _lbsData.longitude = longitude;
    _lbsData.valid = true;
    _lbsData.timestamp = millis();
    _lbsData.state = LBSState::DONE;
    Serial.printf("[Air780EG] LBS定位成功: %.6f, %.6f\n", latitude, longitude);
    return true;
}

lbs_data_t Air780EGModem::getLBSData() {
    return _lbsData;
}

bool Air780EGModem::isLBSDataValid() {
    return !_lastLBSLocation.isEmpty() && (millis() - _lastLBSUpdate < 300000);
}

// AT命令实现
bool Air780EGModem::sendAT(const String& cmd, const String& expected, uint32_t timeout) {
    flushInput();
    _serial.println(cmd);
    return expectResponse(expected, timeout);
}

bool Air780EGModem::sendATWithRetry(const String& cmd, const String& expected, int maxRetry, uint32_t timeout) {
    for (int i = 0; i < maxRetry; i++) {
        if (sendAT(cmd, expected, timeout)) {
            return true;
        }
        delay(500);
    }
    return false;
}

String Air780EGModem::sendATWithResponse(const String& cmd, uint32_t timeout) {
    flushInput();
    _serial.println(cmd);
    return waitResponse(timeout);
}

bool Air780EGModem::sendATThreadSafe(const String& cmd, const String& expected, uint32_t timeout) {
    std::lock_guard<std::mutex> lock(_atMutex);
    return sendAT(cmd, expected, timeout);
}

String Air780EGModem::sendATWithResponseThreadSafe(const String& cmd, uint32_t timeout) {
    std::lock_guard<std::mutex> lock(_atMutex);
    return sendATWithResponse(cmd, timeout);
}

bool Air780EGModem::sendATWithRetryThreadSafe(const String& cmd, const String& expected, int maxRetry, uint32_t timeout) {
    std::lock_guard<std::mutex> lock(_atMutex);
    return sendATWithRetry(cmd, expected, maxRetry, timeout);
}

void Air780EGModem::flushInput() {
    while (_serial.available()) {
        _serial.read();
    }
}

String Air780EGModem::waitResponse(uint32_t timeout) {
    String response = "";
    unsigned long start = millis();
    const size_t MAX_RESPONSE_LENGTH = 2048;
    
    while (millis() - start < timeout) {
        if (_serial.available()) {
            char c = _serial.read();
            
            if (response.length() >= MAX_RESPONSE_LENGTH) {
                break;
            }
            
            response += c;
            
            if (response.endsWith("OK\r\n") || response.endsWith("ERROR\r\n") || 
                response.endsWith("FAIL\r\n") || response.endsWith("> ")) {
                break;
            }
        }
        delay(1);
    }
    
    return response;
}

bool Air780EGModem::expectResponse(const String& expected, uint32_t timeout) {
    String response = waitResponse(timeout);
    return response.indexOf(expected) >= 0;
}

bool Air780EGModem::isReady() {
    return sendAT("AT", "OK", 1000);
}

// 错误处理相关方法
unsigned long Air780EGModem::calculateBackoffDelay() {
    return min(1000UL * (1UL << _cmeErrorCount), 60000UL);
}

bool Air780EGModem::shouldSkipLBSRequest() {
    if (_cmeErrorCount > 0 && millis() - _lastCMEErrorTime < _backoffDelay) {
        return true;
    }
    return false;
}

void Air780EGModem::handleCMEError() {
    _lastCMEErrorTime = millis();
    _cmeErrorCount++;
    _backoffDelay = calculateBackoffDelay();
    debugPrint("Air780EG: CME错误，退避延迟: " + String(_backoffDelay) + "ms");
}

void Air780EGModem::resetCMEErrorCount() {
    _cmeErrorCount = 0;
    _backoffDelay = 0;
}

// 调试功能
void Air780EGModem::debugNetworkInfo() {
    if (!_debug) return;
    
    Serial.println("=== Air780EG 网络信息 ===");
    Serial.println("模块名称: " + getModuleName());
    Serial.println("IMEI: " + getIMEI());
    Serial.println("ICCID: " + getICCID());
    Serial.println("运营商: " + getCarrierName());
    Serial.println("网络类型: " + getNetworkType());
    Serial.println("信号强度: " + String(getCSQ()));
    Serial.println("本地IP: " + getLocalIP());
    Serial.println("网络状态: " + String(_isNetworkReady ? "已连接" : "未连接"));
}

// 后台初始化处理
void Air780EGModem::loop() {
    unsigned long now = millis();
    
    // 处理URC（未请求的结果代码），包括GNSS自动上报数据
    processURC();
    
    // 看门狗检查 - 防止死锁
    if (_lastLoopTime > 0 && (now - _lastLoopTime) > LOOP_TIMEOUT_MS) {
        _loopTimeoutCount++;
        debugPrint("Air780EG: ⚠️ 检测到超时，计数: " + String(_loopTimeoutCount));
        
        if (_loopTimeoutCount >= MAX_TIMEOUT_COUNT) {
            debugPrint("Air780EG: ⚠️ 多次超时，重置初始化状态");
            _initState = INIT_IDLE;
            _loopTimeoutCount = 0;
        }
    }
    _lastLoopTime = now;
    
    // 如果已经完成初始化，直接返回
    if (_initState == INIT_COMPLETED) {
        return;
    }
    
    // 添加安全检查
    if (!_serial) {
        debugPrint("Air780EG: ⚠️ 串口未初始化");
        return;
    }
    
    // 每2秒检查一次
    if (now - _lastInitCheck < 2000) {
        return;
    }
    _lastInitCheck = now;
    
    // 添加异常处理
    try {
        switch (_initState) {
            case INIT_IDLE:
                // 只有在网络未就绪时才开始后台初始化
                if (!isNetworkReady()) {
                    debugPrint("Air780EG: 🔄 开始后台网络注册...");
                    _initState = INIT_WAITING_NETWORK;
                    _initStartTime = now;
                } else {
                    // 网络已就绪，直接跳到GNSS初始化
                    debugPrint("Air780EG: ✅ 网络已就绪，跳过注册步骤");
                    debugPrint("Air780EG: 🛰️ 启用GNSS...");
                    _initState = INIT_ENABLING_GNSS;
                }
                break;
                
            case INIT_WAITING_NETWORK:
                if (isNetworkReady()) {
                    debugPrint("Air780EG: ✅ 网络注册成功");
                    debugPrint("Air780EG: 🛰️ 启用GNSS...");
                    _initState = INIT_ENABLING_GNSS;
                } else if (now - _initStartTime > 60000) { // 60秒超时
                    debugPrint("Air780EG: ⚠️ 网络注册超时，将继续重试");
                    _initStartTime = now; // 重置计时器
                }
                break;
                
            case INIT_ENABLING_GNSS:
                {
                    break;
                    // 添加安全检查，避免在GNSS启用过程中出现异常
                    bool gnssResult = false;
                    try {
                        // gnssResult = enableGNSS(true);
                    } catch (...) {
                        debugPrint("Air780EG: ⚠️ GNSS启用过程中发生异常");
                        gnssResult = false;
                    }
                    
                    if (gnssResult) {
                        debugPrint("Air780EG: ✅ GNSS启用成功");
                        // 安全地设置更新频率
                        try {
                            setGNSSUpdateRate(1);
                        } catch (...) {
                            debugPrint("Air780EG: ⚠️ 设置GNSS更新频率时发生异常");
                        }
                        _initState = INIT_COMPLETED;
                        debugPrint("Air780EG: 🎉 完全初始化完成");
                    } else {
                        debugPrint("Air780EG: ⚠️ GNSS启用失败，将重试");
                        // 继续保持在这个状态，下次再试
                    }
                }
                break;
                
            case INIT_COMPLETED:
                // 初始化完成，不需要做任何事
                break;
                
            default:
                debugPrint("Air780EG: ⚠️ 未知的初始化状态，重置为IDLE");
                _initState = INIT_IDLE;
                break;
        }
    } catch (...) {
        debugPrint("Air780EG: ⚠️ loop函数中发生未捕获的异常，重置状态");
        _initState = INIT_IDLE;
    }
}

// GNSS URC处理
void Air780EGModem::processURC() {
    DEBUG_VERBOSE("Air780EG", "Enter processURC");
    // 检查是否有未读取的数据
    if (!_serial.available()) {
        return;
    }
    
    String buffer = "";
    unsigned long startTime = millis();
    
    // 读取可用数据，但不要阻塞太久
    while (_serial.available() && (millis() - startTime < 100)) {
        char c = _serial.read();
        buffer += c;
        
        // 如果读到完整的一行
        if (c == '\n') {
            buffer.trim();
            if (buffer.length() > 0) {
                // 检查是否是GNSS URC
                if (buffer.startsWith("+UGNSINF:")) {
                    handleGNSSURC(buffer);
                }
                // 判断是否是 mqtt 上报
                if (buffer.startsWith("+MSUB:")) {
                    handleMQTTURC(buffer);
                }
                // 可以在这里添加其他URC处理

            }
            buffer = "";
        }
        
        // 防止缓冲区过大
        if (buffer.length() > 512) {
            buffer = "";
        }
    }
}

bool Air780EGModem::handleGNSSURC(const String& urc) {
    Serial.println("[Air780EG] 收到GNSS URC: " + urc);
    
    // URC格式: +UGNSINF: <数据字段>
    // 这与AT+CGNSINF的响应格式相同，可以复用解析函数
    String fakeResponse = "+CGNSINF: " + urc.substring(10); // 去掉"+UGNSINF: "前缀，加上"+CGNSINF: "
    
    Serial.println("[Air780EG] 转换为标准格式: " + fakeResponse);
    
    if (parseGNSSResponse(fakeResponse)) {
        if (_gnssData.valid) {
            Serial.println("[Air780EG] ✅ URC GNSS数据解析成功，已定位");
        } else {
            Serial.println("[Air780EG] ⏳ URC GNSS数据解析成功，等待定位");
        }
        _lastGNSSUpdate = millis(); // 更新时间戳
        return true;
    } else {
        Serial.println("[Air780EG] ❌ URC GNSS数据解析失败");
        return false;
    }
}

// 分析GNSS数据字段（调试用）
void Air780EGModem::analyzeGNSSFields(const String& data) {
    if (!_debug) return;
    
    Serial.println("=== GNSS字段分析 ===");
    Serial.println("原始数据: " + data);
    int commaCount = 0;
    int lastIndex = 0;
    for (int i = 0; i < 21; ++i) memset(g_gnssFieldsDebug[i], 0, sizeof(g_gnssFieldsDebug[i]));
    for (int i = 0; i < data.length() && commaCount < 20; i++) {
        if (data.charAt(i) == ',') {
            int fieldLen = i - lastIndex;
            strncpy(g_gnssFieldsDebug[commaCount], data.c_str() + lastIndex, std::min(fieldLen, 31));
            g_gnssFieldsDebug[commaCount][std::min(fieldLen, 31)] = '\0';
            lastIndex = i + 1;
            commaCount++;
        }
    }
    if (commaCount < 20) {
        int fieldLen = data.length() - lastIndex;
        strncpy(g_gnssFieldsDebug[commaCount], data.c_str() + lastIndex, std::min(fieldLen, 31));
        g_gnssFieldsDebug[commaCount][std::min(fieldLen, 31)] = '\0';
    }
    
    // 字段说明
    const char* fieldNames[] = {
        "GNSS运行状态", "定位状态", "UTC时间", "纬度", "经度", 
        "海拔高度", "地面速度", "地面航向", "定位模式", "保留1",
        "HDOP", "PDOP", "VDOP", "保留2", "可见卫星数",
        "使用卫星数", "GLONASS卫星数", "保留3", "C/N0最大值", "HPA", "VPA"
    };
    
    Serial.println("字段总数: " + String(commaCount + 1));
    
    for (int i = 0; i <= commaCount && i < 21; i++) {
        String value = g_gnssFieldsDebug[i][0] != '\0' ? String(g_gnssFieldsDebug[i]) : "空";
        Serial.println("字段" + String(i) + " (" + fieldNames[i] + "): " + value);
    }
    
    // 重点字段分析
    if (commaCount >= 15) {
        Serial.println("=== 重点字段分析 ===");
        Serial.println("• GNSS状态: " + String(g_gnssFieldsDebug[0][0] == '1' ? "运行中" : "已停止"));
        Serial.println("• 定位状态: " + String(g_gnssFieldsDebug[1][0] == '1' ? "已定位" : "未定位"));
        Serial.println("• 时间戳: " + (g_gnssFieldsDebug[2][0] != '\0' ? g_gnssFieldsDebug[2] : String("无")));
        Serial.println("• 位置信息: " + String(g_gnssFieldsDebug[3][0] != '\0' && g_gnssFieldsDebug[4][0] != '\0' ? "有效" : "无效"));
        Serial.println("• 可见卫星: " + (g_gnssFieldsDebug[14][0] != '\0' ? g_gnssFieldsDebug[14] : String("0")));
        Serial.println("• 使用卫星: " + (g_gnssFieldsDebug[15][0] != '\0' ? g_gnssFieldsDebug[15] : String("0")));
        Serial.println("• HDOP值: " + (g_gnssFieldsDebug[10][0] != '\0' ? g_gnssFieldsDebug[10] : String("99.9")));
    }
}

// 处理MQTT URC消息
void Air780EGModem::handleMQTTURC(const String& data) {
    debugPrint("[Air780EG] 收到MQTT URC: " + data);
    
    // 安全检查：确保data不为空且包含有效数据
    if (data.length() == 0) {
        debugPrint("[Air780EG] ❌ 收到空消息，忽略");
        return;
    }
    
    // 检查消息格式
    if (data.indexOf("+MSUB:") < 0) {
        debugPrint("[Air780EG] ❌ 消息格式不正确，缺少+MSUB:标识");
        return;
    }
    
    // 安全检查回调函数
    if (_mqttMessageHandler == nullptr) {
        debugPrint("[Air780EG] ❌ MQTT消息处理器未设置，消息将被丢弃");
        return;
    }
    
    debugPrint("[Air780EG] 开始解析MQTT消息内容...");
    
    try {
        // 解析MSUB消息格式: +MSUB: <topic>,<len>,<message>
        // 查找+MSUB:标识
        int msubPos = data.indexOf("+MSUB:");
        if (msubPos < 0) {
            debugPrint("[Air780EG] ❌ 找不到+MSUB:标识");
            return;
        }
        
        // 提取消息部分
        String msgPart = data.substring(msubPos + 6); // 跳过"+MSUB:"
        msgPart.trim();
        debugPrint("[Air780EG] 提取消息部分: " + msgPart);
        
        // 解析格式: <topic>,<len>,<message>
        // 查找第一个逗号 (topic结束位置)
        int firstComma = msgPart.indexOf(',');
        if (firstComma < 0) {
            debugPrint("[Air780EG] ❌ 找不到第一个逗号分隔符");
            return;
        }
        
        // 提取topic
        String topic = msgPart.substring(0, firstComma);
        topic.trim();
        // 去除主题两端的引号
        if (topic.startsWith("\"") && topic.endsWith("\"")) {
            topic = topic.substring(1, topic.length() - 1);
        }
        debugPrint("[Air780EG] 解析到主题: '" + topic + "'");
        
        // 查找第二个逗号 (len结束位置)
        int secondComma = msgPart.indexOf(',', firstComma + 1);
        if (secondComma < 0) {
            debugPrint("[Air780EG] ❌ 找不到第二个逗号分隔符");
            return;
        }
        
        // 提取len (消息长度)
        String lenStr = msgPart.substring(firstComma + 1, secondComma);
        lenStr.trim();
        // 去除可能的"byte"后缀
        if (lenStr.endsWith(" byte")) {
            lenStr = lenStr.substring(0, lenStr.length() - 5);
        }
        int expectedLen = lenStr.toInt();
        debugPrint("[Air780EG] 预期消息长度: " + String(expectedLen));
        
        // 提取message (十六进制数据)
        String hexMessage = msgPart.substring(secondComma + 1);
        hexMessage.trim();
        
        // 清理消息内容，移除换行符和"OK"
        int okPos = hexMessage.indexOf("OK");
        if (okPos >= 0) {
            hexMessage = hexMessage.substring(0, okPos);
        }
        hexMessage.trim();
        
        debugPrint("[Air780EG] 解析到十六进制消息: '" + hexMessage + "'");
        debugPrint("[Air780EG] 十六进制消息长度: " + String(hexMessage.length()));
        
        // 将十六进制字符串转换为实际字符串
        String message = "";
        if (hexMessage.length() % 2 != 0) {
            debugPrint("[Air780EG] ⚠️ 十六进制消息长度不是偶数，可能有问题");
        }
        
        for (int i = 0; i < hexMessage.length(); i += 2) {
            if (i + 1 < hexMessage.length()) {
                String hexByte = hexMessage.substring(i, i + 2);
                // 验证是否为有效的十六进制字符
                bool isValidHex = true;
                for (int j = 0; j < hexByte.length(); j++) {
                    char c = hexByte.charAt(j);
                    if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
                        isValidHex = false;
                        break;
                    }
                }
                
                if (isValidHex) {
                    char c = (char)strtol(hexByte.c_str(), NULL, 16);
                    message += c;
                } else {
                    debugPrint("[Air780EG] ⚠️ 无效的十六进制字符: " + hexByte);
                }
            }
        }
        
        debugPrint("[Air780EG] 转换后的消息: '" + message + "'");
        debugPrint("[Air780EG] 转换后消息长度: " + String(message.length()));
        
        // 验证消息长度
        if (message.length() != expectedLen) {
            debugPrint("[Air780EG] ⚠️ 消息长度不匹配，预期: " + String(expectedLen) + ", 实际: " + String(message.length()));
        }
        
        // 确保topic和message都不为空
        if (topic.length() > 0 && message.length() > 0) {
            debugPrint("[Air780EG] ✅ MQTT消息解析成功，调用消息处理器");
            _mqttMessageHandler(topic, message);
            debugPrint("[Air780EG] 消息处理器执行完成");
        } else {
            debugPrint("[Air780EG] ❌ 主题或消息为空，忽略");
            debugPrint("[Air780EG] 主题长度: " + String(topic.length()));
            debugPrint("[Air780EG] 消息长度: " + String(message.length()));
        }
        
    } catch (...) {
        debugPrint("[Air780EG] ❌ MQTT消息解析异常");
    }
}

// 测试AT命令
void Air780EGModem::testATCommand() {
    Serial.println("=== AT命令测试开始 ===");
    
    // 测试不同波特率
    int baudRates[] = {9600, 115200, 38400, 57600, 19200};
    int numBaudRates = sizeof(baudRates) / sizeof(baudRates[0]);
    
    for (int i = 0; i < numBaudRates; i++) {
        Serial.printf("测试波特率: %d\n", baudRates[i]);
        
        _serial.end();
        delay(100);
        _serial.begin(baudRates[i], SERIAL_8N1, _rxPin, _txPin);
        delay(500);
        
        // 清空缓冲区
        while (_serial.available()) {
            _serial.read();
        }
        
        // 发送AT命令
        _serial.println("AT");
        delay(100);
        
        String response = "";
        unsigned long startTime = millis();
        while (millis() - startTime < 1000) {
            if (_serial.available()) {
                response += (char)_serial.read();
            }
        }
        
        Serial.printf("响应: '%s'\n", response.c_str());
        
        if (response.indexOf("OK") >= 0) {
            Serial.printf("✅ 找到正确波特率: %d\n", baudRates[i]);
            
            // 测试更多AT命令
            Serial.println("测试更多AT命令...");
            
            // 测试模块信息
            _serial.println("ATI");
            delay(500);
            response = "";
            startTime = millis();
            while (millis() - startTime < 1000) {
                if (_serial.available()) {
                    response += (char)_serial.read();
                }
            }
            Serial.printf("模块信息: %s\n", response.c_str());
            
            // 测试SIM卡状态
            _serial.println("AT+CPIN?");
            delay(500);
            response = "";
            startTime = millis();
            while (millis() - startTime < 1000) {
                if (_serial.available()) {
                    response += (char)_serial.read();
                }
            }
            Serial.printf("SIM卡状态: %s\n", response.c_str());
            
            break;
        }
    }
    
    Serial.println("=== AT命令测试结束 ===");
}

#endif // USE_AIR780EG_GSM
