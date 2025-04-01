#include "GPS.h"

// \r\n 是回车换行符的组合
// \r (CR, Carriage Return) - 回车，将光标移动到行首
// \n (LF, Line Feed) - 换行，将光标移动到下一行
// 在串行通信中,\r\n的组合是标准的行结束符
// NMEA协议规定每条GPS语句都必须以\r\n结尾

/*
GGA：时间、位置、卫星数量 
GSA：GPS接收机操作模式，定位使用的卫星，DOP值，定位状态
GSV：可见GPS卫星信息、仰角、方位角、信噪比
RMC：时间、日期、位置、速度
VTG：地面速度信息

$PCAS04,3*1A 北斗和 GPS 双模 
$PCAS04,1*18 单 GPS 工作模式 
$PCAS04,2*1B 单北斗工作模式

// $PCAS03,nGGA,nGLL,nGSA,nGSV,nRMC,nVTG,nZDA,nANT,nDHV,nLPS,res1,res2,nUTC,nGST,res3,res4,res5,nTIM*CS<CR><LF>
// GGA 输出频率，语句输出频率是以定位更新率为基准的，n（0~9）表示每 n 次定位输出一次，0 表示不输出该语句，空则保持原有配置。
// $PCAS03,1,1,1,1,1,1,1,1,0,0,,,1,1,,,,1*33
// RMC,ZDA,nVTG 打开其他都关闭 $PCAS03,0,0,0,0,1,1,1,0,0,0,,,0,0,,,,0*33
// $PCAS06,0*1B 查询产品信息
*/

// GpsCommand类的实现
String GpsCommand::calculateChecksum(const String& cmd) {
    // 移除$符号，只计算$后面的部分
    String data = cmd.substring(1);
    // 如果命令包含*号，去掉*号及其后面的内容
    int starPos = data.indexOf('*');
    if (starPos != -1) {
        data = data.substring(0, starPos);
    }
    
    byte checksum = 0;
    // 计算异或校验和
    for (unsigned int i = 0; i < data.length(); i++) {
        checksum ^= data.charAt(i);
    }
    
    // 转换为十六进制字符串，确保两位
    char checksumStr[3];
    sprintf(checksumStr, "%02X", checksum);
    return String(checksumStr);
}

String GpsCommand::buildBaudrateCmd(int baudRate) {
    String cmd;
    switch (baudRate) {
        case 4800:   cmd = "$PCAS01,0"; break;
        case 9600:   cmd = "$PCAS01,1"; break;
        case 19200:  cmd = "$PCAS01,2"; break;
        case 38400:  cmd = "$PCAS01,3"; break;
        case 57600:  cmd = "$PCAS01,4"; break;
        case 115200: cmd = "$PCAS01,5"; break;
        default:     return ""; // 不支持的波特率返回空字符串
    }
    
    // 计算并添加校验和
    cmd += "*" + calculateChecksum(cmd);
    return cmd + "\r\n";
}

String GpsCommand::buildModeCmd(int mode) {
    String cmd;
    switch (mode) {
        case 1: cmd = "$PCAS04,1"; break; // 单GPS模式
        case 2: cmd = "$PCAS04,2"; break; // 单北斗模式
        case 3: cmd = "$PCAS04,3"; break; // 北斗和GPS双模
        default: return ""; // 不支持的模式返回空字符串
    }
    
    // 计算并添加校验和
    cmd += "*" + calculateChecksum(cmd);
    return cmd + "\r\n";
}

String GpsCommand::buildUpdateRateCmd(int ms) {
    if (ms <= 0) return "";
    
    String cmd = "$PCAS02," + String(ms);
    
    // 计算并添加校验和
    cmd += "*" + calculateChecksum(cmd);
    return cmd + "\r\n";
}

String GpsCommand::buildNmeaSentenceCmd(bool gga, bool gll, bool gsa, bool gsv, 
                                       bool rmc, bool vtg, bool zda) {
    String cmd = "$PCAS03,";
    cmd += gga ? "1," : "0,";
    cmd += gll ? "1," : "0,";
    cmd += gsa ? "1," : "0,";
    cmd += gsv ? "1," : "0,";
    cmd += rmc ? "1," : "0,";
    cmd += vtg ? "1," : "0,";
    cmd += zda ? "1," : "0,";
    // ANT
    cmd += "0,";
    // DHV
    cmd += "0,";
    // LPS
    cmd += "0,";
    // res1, res2
    cmd += ",,";
    // UTC
    cmd += "0,";
    // GST
    cmd += "0,";
    // res3, res4, res5
    cmd += ",,,";
    // TIM
    cmd += "0";
    
    // 计算并添加校验和
    cmd += "*" + calculateChecksum(cmd);
    return cmd + "\r\n";
}

// 添加一个异步初始化标志
bool GPS::_initInProgress = false;

GPS::GPS(int rxPin, int txPin) : gpsSerial(rxPin, txPin)
{
    // 初始化时记录引脚号，便于调试
    _rxPin = rxPin;
    _txPin = txPin;
    _currentHz = 2; // 默认初始值为2Hz
    
    // 初始化数据统计相关变量
    _lastStatTime = 0;
    _validSentences = 0;
    _invalidSentences = 0;
    _lastLocationUpdateTime = 0;
}

// 根据卫星数量自动调整GPS更新频率
bool GPS::autoAdjustUpdateRate()
{
    uint8_t satellites = device.get_gps_data()->satellites;
    int targetHz = 0;
    
    // 根据卫星数量确定目标更新频率，由于数据精简可以设置更高频率
    if (satellites >= 23) {
        targetHz = 10;  // 卫星数量很多，高精度、高更新率
    } else if (satellites >= 18) {
        targetHz = 5;   // 卫星数量中等，中等更新率
    } else if (satellites >= 8) {
        targetHz = 2;   // 卫星数量较少，降低更新率保证质量
    } else {
        targetHz = 1;   // 卫星数量很少，最低更新率以保证质量
    }
    
    // 如果需要调整频率且与当前频率不同
    if (targetHz != _currentHz) {
        Serial.printf("[GPS] 卫星数量: %d，将更新率从 %dHz 调整为 %dHz\n", 
                      satellites, _currentHz, targetHz);
        
        // 暂停当前处理
        bool success = setGpsHz(targetHz);
        
        if (success) {
            _currentHz = targetHz;
            Serial.printf("[GPS] 更新率设置成功: %dHz\n", targetHz);
        } else {
            Serial.println("[GPS] 更新率设置失败");
        }
        
        return success;
    }
    
    return true; // 不需要调整时返回成功
}

// 优化后的命令发送函数
bool GPS::sendGpsCommand(const String& cmd, int retries, int retryDelay) {
    if (cmd.isEmpty()) {
        Serial.println("[GPS] 发送空命令");
        return false;
    }
    
    Serial.print("[GPS] 发送命令 -> ");
    Serial.print(cmd);
    
    // 清空接收缓冲区
    while (gpsSerial.available()) {
        gpsSerial.read();
    }
    
    bool success = false;
    for (int i = 0; i < retries && !success; i++) {
        if (i > 0) {
            Serial.printf("[GPS] 重试发送命令 (%d/%d)\n", i+1, retries);
            delay(100); // 重试前额外延时
        }
        
        gpsSerial.print(cmd);
        
        // 等待发送完成
        gpsSerial.flush();
        
        // 增加延时等待GPS模块处理，避免通信不稳定
        delay(retryDelay * 2);
        
        // 检查是否收到响应数据（简单检测）
        unsigned long startTime = millis();
        bool receivedData = false;
        
        // 等待最多500ms检查是否有数据返回
        while (millis() - startTime < 500) {
            if (gpsSerial.available()) {
                receivedData = true;
                break;
            }
            delay(10);
        }
        
        // 如果收到了任何响应或这是最后一次尝试，就认为成功
        success = receivedData || (i == retries - 1);
    }
    
    if (success) {
        Serial.println("[GPS] 命令发送成功");
    } else {
        Serial.println("[GPS] 命令发送失败");
    }
    
    return success;
}

// 保留旧方法以兼容现有代码
void GPS::configGps(const char *configCmd)
{
    sendGpsCommand(String(configCmd));
}

void GPS::begin()
{
    // 基础串口初始化
    gpsSerial.begin(9600);
    
    // 创建异步初始化任务
    xTaskCreate(
        [](void* param) {
            GPS* gps = (GPS*)param;
            gps->asyncInit();
            vTaskDelete(NULL);
        },
        "GPS_Init",
        4096,
        this,
        1,
        NULL
    );
}

void GPS::asyncInit()
{
    _initInProgress = true;
    
    // 执行原来的初始化步骤
    configGpsMode();
    setGpsHz(5);
    
    _initInProgress = false;
    Serial.println("[GPS] 异步初始化完成");
}

// 配置GPS工作模式，启用双模式和完整NMEA输出
void GPS::configGpsMode()
{
    // 减少初始等待时间
    delay(100);
    
    // 设置为北斗和GPS双模
    Serial.println("[GPS] 设置为GPS+北斗双模模式");
    
    // 减少重试次数和间隔
    if(sendGpsCommand(GpsCommand::buildModeCmd(3), 1, 100)) {
        Serial.println("[GPS] 双模设置成功");
    } else {
        Serial.println("[GPS] 警告：双模设置失败");
    }
    
    // 减少延时
    delay(100);
    
    // 设置NMEA语句输出
    Serial.println("[GPS] 设置精简NMEA语句输出");
    
    // 减少重试次数和间隔
    if(sendGpsCommand(GpsCommand::buildNmeaSentenceCmd(true, false, true, false, true, false, false), 1, 100)) {
        Serial.println("[GPS] NMEA语句精简设置成功");
    } else {
        Serial.println("[GPS] 警告：NMEA语句设置失败");
    }
    
    // 清空GPS接收缓冲区
    while(gpsSerial.available()) {
        gpsSerial.read();
    }
}

/*
检查 GPS 天线 状态
$GPTXT,01,01,01,ANTENNA OPEN*25
表示天线状态（开路）
$GPTXT,01,01,01,ANTENNA OK*35
表示天线状态（良好）
$GPTXT,01,01,01,ANTENNA SHORT*63
表示天线状态（短路）
*/
bool GPS::checkGpsStatus()
{
    // TODO
    return true;
}

// 优化后的频率设置函数
bool GPS::setGpsHz(int hz)
{
    int updateRate = 0;
    switch (hz) {
        case 1:  updateRate = 1000; break;
        case 2:  updateRate = 500;  break;
        case 5:  updateRate = 200;  break;
        case 10: updateRate = 100;  break;
        default: return false;
    }
    
    Serial.printf("[GPS] 尝试设置更新频率为%dHz (周期%dms)\n", hz, updateRate);
    
    // 在9600波特率下，使用精简NMEA消息可以支持更高更新率
    if (hz > 10) {
        Serial.println("[GPS] 警告：即使精简NMEA消息，超过10Hz仍可能不稳定");
    }
    
    bool success = false;
    
    // 减少重试次数和间隔
    success = sendGpsCommand(GpsCommand::buildUpdateRateCmd(updateRate), 1, 100);
    
    if (success) {
        device.get_device_state()->gpsHz = hz;
        Serial.printf("[GPS] 更新率成功设置为%dHz\n", hz);
    } else {
        Serial.println("[GPS] 更新率设置失败，保持原有配置");
    }
    
    return success;
}

// 在loop中检查初始化状态
void GPS::loop()
{
    // 如果正在初始化，跳过数据处理
    if (_initInProgress) {
        return;
    }
    
    // 增大缓冲区以处理更高频率数据
    const int bufferSize = 256;
    char buffer[bufferSize];
    int charsRead = 0;
    
    // 一次性读取多个字符进缓冲区，减少频繁读取的开销
    while (gpsSerial.available() && charsRead < bufferSize - 1) {
        buffer[charsRead++] = (char)gpsSerial.read();
        
        // 快速读取，不添加延时
        // 由于只选择了必要的NMEA语句，数据量减少，可以支持更高频率
    }
    
    // 添加字符串终止符
    if (charsRead > 0) {
        buffer[charsRead] = '\0';
        
        // 逐个字符处理
        for (int i = 0; i < charsRead; i++) {
            // 跟踪GPS解析成功和失败的次数
            if (gps.encode(buffer[i])) {
                // 有效数据处理
                updateGpsData();
            } else if (buffer[i] == '\n') {
                // 每当遇到行结束符，根据之前累积的数据检测是否有一个完整有效的语句
                if (gps.failedChecksum() > 0) {
                    _invalidSentences++;
                } else if (gps.passedChecksum() > 0) {
                    _validSentences++;
                }
            }
            
            // 记录位置更新时间
            if (gps.location.isUpdated()) {
                _lastLocationUpdateTime = millis();
            }
        }
    }
}

// 从TinyGPS++更新设备GPS数据
void GPS::updateGpsData()
{
    if (gps.location.isUpdated()) {
        device.get_gps_data()->latitude = gps.location.lat();
        device.get_gps_data()->longitude = gps.location.lng();
    }

    if (gps.altitude.isUpdated()) {
        device.get_gps_data()->altitude = gps.altitude.meters();
    }

    if (gps.course.isUpdated()) {
        device.get_gps_data()->heading = gps.course.deg();
    }

    if (gps.time.isUpdated()) {
        device.get_gps_data()->hour = gps.time.hour();
        device.get_gps_data()->minute = gps.time.minute();
        device.get_gps_data()->second = gps.time.second();
        device.get_gps_data()->centisecond = gps.time.centisecond();
    }

    if (gps.date.isUpdated()) {
        device.get_gps_data()->year = gps.date.year();
        device.get_gps_data()->month = gps.date.month();
        device.get_gps_data()->day = gps.date.day();
    }
    
    if (gps.hdop.isUpdated()) {
        device.get_gps_data()->hdop = gps.hdop.value();
    }
    
    if (gps.satellites.isUpdated()) {
        device.get_gps_data()->satellites = gps.satellites.value();
    }
    
    if (gps.speed.isUpdated()) {
        device.get_gps_data()->speed = gps.speed.kmph();
    }
}

// 打印原始数据
void GPS::printRawData()
{
    // 增大缓冲区以处理更高频率数据
    const int bufferSize = 256;
    char buffer[bufferSize];
    int charsRead = 0;
    
    // 一次性读取多个字符
    while (gpsSerial.available() && charsRead < bufferSize - 1) {
        buffer[charsRead] = (char)gpsSerial.read();
        charsRead++;
        
        // 快速读取，不添加延时
    }
    
    // 添加字符串终止符并打印
    if (charsRead > 0) {
        buffer[charsRead] = '\0';
        Serial.print(buffer);
    }
}

// 打印GPS数据接收统计信息
void GPS::printGpsStats() 
{
    unsigned long now = millis();
    unsigned long elapsedTime = now - _lastStatTime;
    
    // 每5秒打印一次统计信息
    if (elapsedTime >= 5000) {
        // 计算当前周期内的句子接收速率
        float validRate = _validSentences * 1000.0f / elapsedTime;
        float invalidRate = _invalidSentences * 1000.0f / elapsedTime;
        
        // 计算自上次位置更新以来的时间
        unsigned long timeSinceLastFix = now - _lastLocationUpdateTime;
        
        Serial.println("\n=== GPS接收统计信息 ===");
        Serial.printf("有效NMEA语句: %lu (%.1f句/秒)\n", _validSentences, validRate);
        Serial.printf("无效NMEA语句: %lu (%.1f句/秒)\n", _invalidSentences, invalidRate);
        
        // 配置的更新率与实际接收对比
        Serial.printf("配置更新率: %dHz，实际位置更新率: %.1fHz\n", 
                    _currentHz, 
                    (_lastLocationUpdateTime > 0) ? (1000.0f / max(timeSinceLastFix, 1UL)) : 0.0f);
        
        Serial.printf("当前卫星数量: %d\n", device.get_gps_data()->satellites);
        Serial.printf("HDOP: %.1f\n", device.get_gps_data()->hdop);
        Serial.printf("自上次位置更新经过时间: %lu ms\n", timeSinceLastFix);
        Serial.println("======================\n");
        
        // 重置统计数据
        _lastStatTime = now;
        _validSentences = 0;
        _invalidSentences = 0;
    }
}