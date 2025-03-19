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

GPS::GPS(int rxPin, int txPin) : gpsSerial(rxPin, txPin)
{
    // 初始化时记录引脚号，便于调试
    _rxPin = rxPin;
    _txPin = txPin;
    _currentHz = 2; // 默认初始值为2Hz
}

// 根据卫星数量自动调整GPS更新频率
bool GPS::autoAdjustUpdateRate()
{
    uint8_t satellites = device.get_gps_data()->satellites;
    int targetHz = 0;
    
    // 根据卫星数量确定目标更新频率
    if (satellites >= 20) {
        targetHz = 10;  // 卫星数量很多，高精度、高更新率
    } else if (satellites >= 10) {
        targetHz = 5;   // 卫星数量中等，中等更新率
    } else if (satellites >= 4) {
        targetHz = 2;   // 卫星数量较少，默认更新率
    } else {
        targetHz = 1;   // 卫星数量很少，低更新率以保证质量
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
        Serial.println("GPS: 发送空命令");
        return false;
    }
    
    Serial.print("GPS: 发送命令 -> ");
    Serial.print(cmd);
    
    bool success = false;
    for (int i = 0; i < retries && !success; i++) {
        if (i > 0) {
            Serial.printf("GPS: 重试发送命令 (%d/%d)\n", i+1, retries);
        }
        
        gpsSerial.print(cmd);
        
        // 等待发送完成
        gpsSerial.flush();
        
        // 添加延时等待GPS模块处理
        delay(retryDelay);
        
        // 模块端的响应处理可以在这里添加
        // 对于某些命令，GPS模块会发送ACK，可以在这里读取并验证
        
        // 简单起见，我们假设发送成功
        success = true;
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
    Serial.println(TinyGPSPlus::libraryVersion());
    Serial.printf("GPS使用引脚 RX:%d TX:%d\n", _rxPin, _txPin);
    
    // 初始默认波特率为9600，但要设置为115200
    gpsSerial.begin(9600);  // 初始波特率
    delay(200); // 增加延时确保GPS模块稳定
    
    // 尝试多次设置波特率为115200
    if(setGpsBaudRate(115200)) {
        Serial.println("GPS波特率设置为: 115200");
    } else {
        Serial.println("GPS波特率设置失败，尝试默认波特率");
        // 如果设置115200失败，可能模块已经是115200，重新初始化
        gpsSerial.end();
        delay(100);
        gpsSerial.begin(115200);
    }
    delay(200);

    setMode1();
    delay(200);
    
    // 设置默认2Hz更新率
    if(setGpsHz(2)) {
        _currentHz = 2;
        Serial.println("GPS默认更新率设置为: 2Hz");
    }
    
    delay(200);
    Serial.println("GPS 初始化完成");
}

void GPS::setMode1()
{
    // 北斗 单模
    // sendGpsCommand(GpsCommand::buildModeCmd(2));
    // GPS 单模
    sendGpsCommand(GpsCommand::buildModeCmd(1));
    // 1000ms 输出一次 RMC 消息
    // sendGpsCommand(GpsCommand::buildNmeaSentenceCmd(false, false, false, false, true, true, true));
}

void GPS::setMode2()
{
    // 双模式
    sendGpsCommand(GpsCommand::buildModeCmd(3));
    // sendGpsCommand(GpsCommand::buildNmeaSentenceCmd(false, false, false, false, false, false, true));
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
    
    bool success = sendGpsCommand(GpsCommand::buildUpdateRateCmd(updateRate));
    
    // 只有在成功时才更新设备中的gpsHz值
    if (success) {
        device.get_device_state()->gpsHz = hz;
    }
    
    return success;
}

// 优化波特率设置函数
bool GPS::setGpsBaudRate(int baudRate)
{
    // 使用新的命令构建器生成波特率设置命令
    String baudrateCmd = GpsCommand::buildBaudrateCmd(baudRate);
    if (baudrateCmd.isEmpty()) {
        Serial.printf("GPS: 不支持的波特率 %d\n", baudRate);
        return false;
    }
    
    // 发送命令
    bool success = sendGpsCommand(baudrateCmd, 5);
    
    // 如果发送成功，重新配置串口波特率
    if (success) {
        gpsSerial.end();
        delay(100);
        gpsSerial.begin(baudRate);
    }
    
    return success;
}

// 无延迟的 loop 才能准确拿到数据，必须要执行
void GPS::loop()
{
    while (gpsSerial.available() > 0)
    {
        char c = (char)gpsSerial.read();
        if (gps.encode(c))
        {
            if (gps.location.isUpdated())
            {
                // 使用更高效的时间获取方式
                auto loc = gps.location;
                device.get_gps_data()->latitude = loc.lat();
                device.get_gps_data()->longitude = loc.lng();
            }

            if (gps.altitude.isUpdated())
            {
                auto alt = gps.altitude;
                device.get_gps_data()->altitude = alt.meters();
            }

            if (gps.course.isUpdated())
            {
                auto course = gps.course;
                device.get_gps_data()->heading = course.deg();
            }

            if (gps.time.isUpdated())
            {
                auto time = gps.time;
                device.get_gps_data()->hour = time.hour();
                device.get_gps_data()->minute = time.minute();
                device.get_gps_data()->second = time.second();
                device.get_gps_data()->centisecond = time.centisecond();
            }

            if (gps.date.isUpdated())
            {
                auto date = gps.date;
                device.get_gps_data()->year = date.year();
                device.get_gps_data()->month = date.month();
                device.get_gps_data()->day = date.day();
            }
            
            if (gps.hdop.isUpdated())
            {
                // 获取
                auto hdop = gps.hdop;
                device.get_gps_data()->hdop = hdop.value();
            }
            
            if (gps.satellites.isUpdated())
            {
                auto satellites = gps.satellites;
                device.get_gps_data()->satellites = satellites.value();
            }
            
            if (gps.speed.isUpdated())
            {
                auto speed = gps.speed;
                device.get_gps_data()->speed = speed.kmph();
            }
            
        }
    }
}

// 打印原始数据
void GPS::printRawData()
{
    while (gpsSerial.available() > 0)
    {
        char c = (char)gpsSerial.read();
        
        Serial.print(c);
    }
}