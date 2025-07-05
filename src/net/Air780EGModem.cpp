/*
 * Air780EG 4G+GNSS Modem 驱动实现
 */

#include "Air780EGModem.h"

#ifdef USE_AIR780EG_GSM

#include <HardwareSerial.h>

// 全局实例声明
Air780EGModem air780eg_modem(Serial2, GSM_RX_PIN, GSM_TX_PIN, GSM_EN);

Air780EGModem::Air780EGModem(HardwareSerial& serial, int rxPin, int txPin, int enPin)
    : _serial(serial), _rxPin(rxPin), _txPin(txPin), _enPin(enPin),
      _debug(false), _lbsEnabled(false), _gnssEnabled(false),
      _isNetworkReady(false), _lastNetworkReadyCheckTime(0),
      _initState(INIT_IDLE), _initStartTime(0), _lastInitCheck(0),
      _lastLBSUpdate(0), _isLBSLoading(false),
      _lastCMEErrorTime(0), _cmeErrorCount(0), _backoffDelay(0),
      _lastGNSSUpdate(0), _gnssUpdateRate(1),
      _lastLoopTime(0), _loopTimeoutCount(0) {
}

bool Air780EGModem::begin(uint32_t baudrate) {
    debugPrint("Air780EG: 初始化开始");
    
    // 配置使能引脚
    if (_enPin >= 0) {
        debugPrint("Air780EG: 配置使能引脚 GPIO" + String(_enPin));
        pinMode(_enPin, OUTPUT);
        
        // 先拉低再拉高，确保模块重启
        digitalWrite(_enPin, LOW);
        debugPrint("Air780EG: GSM_EN -> LOW");
        delay(500);  // 增加延时确保模块完全断电
        
        digitalWrite(_enPin, HIGH);
        debugPrint("Air780EG: GSM_EN -> HIGH");
        delay(2000); // 等待模块启动
        
        // 验证引脚状态
        bool pinState = digitalRead(_enPin);
        debugPrint("Air780EG: GSM_EN引脚状态: " + String(pinState ? "HIGH" : "LOW"));
        
        if (!pinState) {
            debugPrint("Air780EG: 警告 - GSM_EN引脚未能拉高！");
            return false;
        }
    } else {
        debugPrint("Air780EG: 警告 - 未配置使能引脚");
    }
    
    // 初始化串口
    debugPrint("Air780EG: 初始化串口 - RX:" + String(_rxPin) + ", TX:" + String(_txPin));
    _serial.begin(baudrate, SERIAL_8N1, _rxPin, _txPin);
    delay(1000);
    
    // 尝试不同波特率
    if (!tryBaudrate(baudrate)) {
        debugPrint("Air780EG: 尝试其他波特率");
        uint32_t baudrates[] = {115200, 9600, 38400, 57600};
        for (int i = 0; i < 4; i++) {
            if (baudrates[i] != baudrate && tryBaudrate(baudrates[i])) {
                debugPrint("Air780EG: 找到正确波特率: " + String(baudrates[i]));
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
    debugPrint("Air780EG: 初始化模块");
    
    // 基础AT命令测试
    if (!sendATWithRetry("AT", "OK", 5, 2000)) {
        debugPrint("Air780EG: AT命令失败");
        return false;
    }
    
    // 关闭回显
    sendAT("ATE0");
    
    // 获取模块信息
    String moduleName = getModuleName();
    debugPrint("Air780EG: 模块名称: " + moduleName);
    
    // 检查SIM卡
    if (!sendATWithRetry("AT+CPIN?", "READY", 3, 3000)) {
        debugPrint("Air780EG: SIM卡未就绪");
        return false;
    }
    
    // 设置网络注册检查
    sendAT("AT+CREG=1");  // 启用网络注册状态上报
    sendAT("AT+CGREG=1"); // 启用GPRS注册状态上报
    
    // 等待网络注册
    debugPrint("Air780EG: 等待网络注册...");
    for (int i = 0; i < 30; i++) {
        if (isNetworkReadyCheck()) {
            debugPrint("Air780EG: 网络注册成功");
            break;
        }
        delay(2000);
    }
    
    debugPrint("Air780EG: 初始化完成");
    
    // 启动后台初始化状态机
    _initState = INIT_IDLE;
    _initStartTime = millis();
    _lastInitCheck = 0;
    
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

void Air780EGModem::debugPrint(const String& msg) {
    if (_debug) {
        GNSS_DEBUG_PRINTLN("[Air780EG] " + msg);
    }
}

bool Air780EGModem::isNetworkReadyCheck() {
    // 避免频繁检查
    if (millis() - _lastNetworkReadyCheckTime < 5000) {
        return _isNetworkReady;
    }
    
    _lastNetworkReadyCheckTime = millis();
    
    // 检查网络注册状态
    String response = sendATWithResponse("AT+CREG?", 2000);
    if (response.indexOf("+CREG: 1,1") >= 0 || response.indexOf("+CREG: 1,5") >= 0) {
        _isNetworkReady = true;
        return true;
    }
    
    // 检查GPRS注册状态
    response = sendATWithResponse("AT+CGREG?", 2000);
    if (response.indexOf("+CGREG: 1,1") >= 0 || response.indexOf("+CGREG: 1,5") >= 0) {
        _isNetworkReady = true;
        return true;
    }
    
    _isNetworkReady = false;
    return false;
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
    // 提取15位IMEI号码
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
        GNSS_DEBUG_PRINTLN("[Air780EG] 启用GNSS");
        
        // 先查询当前GNSS状态
        String currentStatus = sendATWithResponse("AT+CGNSPWR?", 3000);
        if (currentStatus.length() > 0) {
            GNSS_DEBUG_PRINTLN("[Air780EG] 当前GNSS状态: " + currentStatus);
        }
        
        // 1. 开启GNSS电源
        GNSS_DEBUG_PRINTLN("[Air780EG] 发送GNSS开启命令...");
        if (sendAT("AT+CGNSPWR=1", "OK", 5000)) {
            GNSS_DEBUG_PRINTLN("[Air780EG] GNSS电源开启成功");
            _gnssEnabled = true;
            delay(2000); // 等待GNSS启动
            
            // 2. 启用位置辅助定位 - 关键步骤，可大幅缩短定位时间
            GNSS_DEBUG_PRINTLN("[Air780EG] 启用位置辅助定位...");
            if (sendAT("AT+CGNSAID=31,1,1,1", "OK", 3000)) {
                GNSS_DEBUG_PRINTLN("[Air780EG] 位置辅助定位启用成功");
            } else {
                GNSS_DEBUG_PRINTLN("[Air780EG] ⚠️ 位置辅助定位启用失败");
            }
            
            // 3. 设置定位信息自动上报（每隔5个fix上报一次）
            GNSS_DEBUG_PRINTLN("[Air780EG] 设置定位信息自动上报...");
            if (sendAT("AT+CGNSURC=1", "OK", 3000)) {
                GNSS_DEBUG_PRINTLN("[Air780EG] 自动上报设置成功");
            } else {
                GNSS_DEBUG_PRINTLN("[Air780EG] ⚠️ 自动上报设置失败");
            }
            
            // 3. 验证GNSS状态
            String verifyStatus = sendATWithResponse("AT+CGNSPWR?", 3000);
            if (verifyStatus.length() > 0) {
                debugPrint("Air780EG: GNSS启用后状态: " + verifyStatus);
            }
            
            // 4. 获取GNSS信息进行调试（简化版本）
            delay(1000);
            String gnssInfo = sendATWithResponse("AT+CGNSINF", 3000);
            if (gnssInfo.length() > 0 && gnssInfo.indexOf("+CGNSINF:") >= 0) {
                debugPrint("Air780EG: GNSS信息获取成功");
                
                // 简化的解析，避免复杂的字符串操作
                int dataStart = gnssInfo.indexOf(":") + 1;
                if (dataStart > 0 && dataStart < gnssInfo.length()) {
                    String data = gnssInfo.substring(dataStart);
                    data.trim();
                    
                    // 只检查前两个字段
                    int firstComma = data.indexOf(',');
                    if (firstComma > 0 && firstComma < data.length() - 1) {
                        String powerStatus = data.substring(0, firstComma);
                        
                        int secondComma = data.indexOf(',', firstComma + 1);
                        if (secondComma > firstComma && secondComma < data.length()) {
                            String fixStatus = data.substring(firstComma + 1, secondComma);
                            
                            debugPrint("Air780EG: GNSS电源: " + String(powerStatus == "1" ? "开启" : "关闭"));
                            debugPrint("Air780EG: 定位状态: " + String(fixStatus == "1" ? "已定位" : "未定位"));
                            
                            if (fixStatus == "0") {
                                debugPrint("Air780EG: ⚠️ GNSS定位建议:");
                                debugPrint("  • 确保设备在室外或靠近窗户");
                                debugPrint("  • 检查GNSS天线连接");
                                debugPrint("  • 冷启动可能需要5-15分钟");
                            }
                        }
                    }
                }
            } else {
                debugPrint("Air780EG: ⚠️ 无法获取GNSS信息");
            }
            
            debugPrint("Air780EG: GNSS启用成功");
            return true;
        } else {
            debugPrint("Air780EG: GNSS电源开启失败");
            
            // 获取详细错误信息（简化版本）
            String errorInfo = sendATWithResponse("AT+CMEE=2", 3000);
            if (errorInfo.length() > 0) {
                debugPrint("Air780EG: 启用详细错误信息: " + errorInfo);
            }
            
            // 再次尝试查询状态
            String statusAfterFail = sendATWithResponse("AT+CGNSPWR?", 3000);
            if (statusAfterFail.length() > 0) {
                debugPrint("Air780EG: 失败后GNSS状态: " + statusAfterFail);
            }
            
            return false;
        }
    } else {
        debugPrint("Air780EG: 禁用GNSS");
        
        // 1. 关闭定位信息自动上报
        sendAT("AT+CGNSURC=0", "OK", 3000);
        
        // 2. 关闭GNSS电源
        if (sendAT("AT+CGNSPWR=0", "OK", 3000)) {
            _gnssEnabled = false;
            debugPrint("Air780EG: GNSS禁用成功");
            return true;
        } else {
            debugPrint("Air780EG: GNSS禁用失败");
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
        String fields[22];
        
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

// 检查GNSS是否正在等待定位（电源开启但未定位）
bool Air780EGModem::isGNSSWaitingFix() {
    String response = sendATWithResponse("AT+CGNSINF", 2000);
    if (response.length() > 0) {
        int firstComma = response.indexOf(',');
        int secondComma = response.indexOf(',', firstComma + 1);
        if (firstComma > 0 && secondComma > 0) {
            String powerStatus = response.substring(response.indexOf(':') + 1, firstComma);
            String fixStatus = response.substring(firstComma + 1, secondComma);
            powerStatus.trim();
            fixStatus.trim();
            
            // GNSS电源开启但未定位
            return (powerStatus == "1" && fixStatus == "0");
        }
    }
    return false;
}

bool Air780EGModem::updateGNSSData() {
    if (!_gnssEnabled) return false;
    
    // 避免过于频繁的更新
    unsigned long interval = 1000 / _gnssUpdateRate;
    if (millis() - _lastGNSSUpdate < interval) {
        return _gnssData.valid;
    }
    
    _lastGNSSUpdate = millis();
    
    String response = sendATWithResponse("AT+CGNSINF", 3000);
    if (response.length() > 0) {
        _lastGNSSRaw = response;
        return parseGNSSResponse(response);
    }
    
    return false;
}

bool Air780EGModem::parseGNSSResponse(const String& response) {
    // Air780EG GNSS响应格式: +CGNSINF: <GNSS run status>,<Fix status>,<UTC date & Time>,<Latitude>,<Longitude>,<MSL Altitude>,<Speed Over Ground>,<Course Over Ground>,<Fix Mode>,<Reserved1>,<HDOP>,<PDOP>,<VDOP>,<Reserved2>,<GNSS Satellites in View>,<GNSS Satellites Used>,<GLONASS Satellites Used>,<Reserved3>,<C/N0 max>,<HPA>,<VPA>
    
    int start = response.indexOf("+CGNSINF: ") + 10;
    if (start < 10) return false;
    
    String data = response.substring(start);
    data.trim();
    
    // 分割数据
    int commaCount = 0;
    int lastIndex = 0;
    String fields[21];
    
    for (int i = 0; i < data.length() && commaCount < 20; i++) {
        if (data.charAt(i) == ',') {
            fields[commaCount] = data.substring(lastIndex, i);
            lastIndex = i + 1;
            commaCount++;
        }
    }
    if (commaCount < 20) {
        fields[commaCount] = data.substring(lastIndex);
    }
    
    // 解析字段
    bool gnssRunning = fields[0].toInt() == 1;
    bool fixStatus = fields[1].toInt() == 1;
    
    if (!gnssRunning || !fixStatus) {
        _gnssData.valid = false;
        return false;
    }
    
    _gnssData.valid = true;
    _gnssData.timestamp = fields[2];
    _gnssData.latitude = fields[3].toFloat();
    _gnssData.longitude = fields[4].toFloat();
    _gnssData.altitude = fields[5].toFloat();
    _gnssData.speed = fields[6].toFloat();
    _gnssData.course = fields[7].toFloat();
    _gnssData.fix_mode = fields[8];
    _gnssData.hdop = fields[10].toFloat();
    _gnssData.satellites = fields[15].toInt();
    
    GNSS_DEBUG_PRINTF("[Air780EG] GNSS数据: Lat=%.6f, Lon=%.6f, Sats=%d\n",
                      _gnssData.latitude, _gnssData.longitude, _gnssData.satellites);
    
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
        _gnssUpdateRate = hz;
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
        
        gpsData.gpsHz = _gnssUpdateRate;
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
        LBS_DEBUG_PRINTLN("[Air780EG] 启用LBS定位");
        // Air780EG的LBS配置可能与ml307不同，需要查阅具体AT指令
        return sendAT("AT+CLBS=1", "OK", 3000);
    } else {
        LBS_DEBUG_PRINTLN("[Air780EG] 禁用LBS定位");
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
        return !_lastLBSLocation.isEmpty();
    }
    
    if (shouldSkipLBSRequest()) {
        return false;
    }
    
    _isLBSLoading = true;
    _lastLBSUpdate = millis();
    
    String response = sendATWithResponse("AT+CLBS=1", 10000);
    _isLBSLoading = false;
    
    if (response.length() > 0) {
        _lastLBSLocation = response;
        return parseLBSResponse(response);
    }
    
    return false;
}

bool Air780EGModem::parseLBSResponse(const String& response) {
    // 这里需要根据Air780EG的实际LBS响应格式来解析
    // 暂时使用简单的解析逻辑
    if (response.indexOf("OK") >= 0) {
        return true;
    }
    return false;
}

lbs_data_t Air780EGModem::getLBSData() {
    lbs_data_t lbsData;
    // 实现LBS数据解析
    return lbsData;
}

bool Air780EGModem::isLBSDataValid() {
    return !_lastLBSLocation.isEmpty() && (millis() - _lastLBSUpdate < 300000);
}

// AT命令实现
bool Air780EGModem::sendAT(const String& cmd, const String& expected, uint32_t timeout) {
    flushInput();
    _serial.println(cmd);
    
    if (_debug) {
        Serial.println(">> " + cmd);
    }
    
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
    
    if (_debug) {
        Serial.println(">> " + cmd);
    }
    
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
    const size_t MAX_RESPONSE_LENGTH = 2048; // 限制响应长度，防止内存溢出
    
    while (millis() - start < timeout) {
        if (_serial.available()) {
            char c = _serial.read();
            
            // 检查响应长度，防止内存溢出
            if (response.length() >= MAX_RESPONSE_LENGTH) {
                debugPrint("Air780EG: ⚠️ 响应过长，截断处理");
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
    
    if (_debug && response.length() > 0) {
        // 如果响应太长，只显示前面和后面的部分
        if (response.length() > 200) {
            String truncated = response.substring(0, 100) + "...[截断]..." + 
                              response.substring(response.length() - 100);
            Serial.println("<< " + truncated);
        } else {
            Serial.println("<< " + response);
        }
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

void Air780EGModem::debugGNSSInfo() {
    if (!_debug) return;
    
    GNSS_DEBUG_PRINTLN("=== Air780EG GNSS信息 ===");
    
    // 查询GNSS电源状态
    String powerStatus = sendATWithResponse("AT+CGNSPWR?", 2000);
    GNSS_DEBUG_PRINTLN("GNSS电源状态: " + powerStatus);
    
    // 查询详细GNSS信息
    String gnssInfo = sendATWithResponse("AT+CGNSINF", 3000);
    GNSS_DEBUG_PRINTLN("GNSS详细信息: " + gnssInfo);
    
    GNSS_DEBUG_PRINTLN("本地解析状态:");
    GNSS_DEBUG_PRINTLN("  GNSS启用: " + String(_gnssEnabled ? "是" : "否"));
    GNSS_DEBUG_PRINTLN("  定位有效: " + String(_gnssData.valid ? "是" : "否"));
    
    if (_gnssData.valid) {
        GNSS_DEBUG_PRINTLN("  纬度: " + String(_gnssData.latitude, 6));
        GNSS_DEBUG_PRINTLN("  经度: " + String(_gnssData.longitude, 6));
        GNSS_DEBUG_PRINTLN("  海拔: " + String(_gnssData.altitude, 2) + "m");
        GNSS_DEBUG_PRINTLN("  速度: " + String(_gnssData.speed, 2) + "km/h");
        GNSS_DEBUG_PRINTLN("  方向: " + String(_gnssData.course, 2) + "°");
        GNSS_DEBUG_PRINTLN("  卫星数: " + String(_gnssData.satellites));
        GNSS_DEBUG_PRINTLN("  HDOP: " + String(_gnssData.hdop, 2));
        GNSS_DEBUG_PRINTLN("  时间戳: " + _gnssData.timestamp);
        GNSS_DEBUG_PRINTLN("  定位模式: " + _gnssData.fix_mode);
    } else if (_gnssEnabled) {
        GNSS_DEBUG_PRINTLN("  状态: 等待定位中...");
        GNSS_DEBUG_PRINTLN("  建议: 确保设备在室外，等待5-15分钟");
    }
    
    GNSS_DEBUG_PRINTLN("  更新频率: " + String(_gnssUpdateRate) + "Hz");
    GNSS_DEBUG_PRINTLN("  最后更新: " + String(millis() - _lastGNSSUpdate) + "ms前");
}

void Air780EGModem::debugLBSConfig() {
    if (!_debug) return;
    
    LBS_DEBUG_PRINTLN("=== Air780EG LBS信息 ===");
    LBS_DEBUG_PRINTLN("LBS状态: " + String(_lbsEnabled ? "已启用" : "未启用"));
    LBS_DEBUG_PRINTLN("最后更新: " + String(millis() - _lastLBSUpdate) + "ms前");
    LBS_DEBUG_PRINTLN("原始数据: " + _lastLBSLocation);
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
                    // 添加安全检查，避免在GNSS启用过程中出现异常
                    bool gnssResult = false;
                    try {
                        gnssResult = enableGNSS(true);
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
    GNSS_DEBUG_PRINTLN("[Air780EG] 收到GNSS URC: " + urc);
    
    // URC格式: +UGNSINF: <数据字段>
    // 这与AT+CGNSINF的响应格式相同，可以复用解析函数
    String fakeResponse = "+CGNSINF: " + urc.substring(10); // 去掉"+UGNSINF: "前缀，加上"+CGNSINF: "
    
    if (parseGNSSResponse(fakeResponse)) {
        GNSS_DEBUG_PRINTLN("[Air780EG] URC GNSS数据解析成功");
        _lastGNSSUpdate = millis(); // 更新时间戳
        return true;
    } else {
        GNSS_DEBUG_PRINTLN("[Air780EG] URC GNSS数据解析失败");
        return false;
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
