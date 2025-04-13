#include "GPS.h"
#include "device.h"
#include <TinyGPS++.h>

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

/*
检查 GPS 天线 状态
$GPTXT,01,01,01,ANTENNA OPEN*25
表示天线状态（开路）
$GPTXT,01,01,01,ANTENNA OK*35
表示天线状态（良好）
$GPTXT,01,01,01,ANTENNA SHORT*63
表示天线状态（短路）
*/

// 定义常用的GPS波特率数组
const uint32_t BAUD_RATES[] = {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};
const int NUM_BAUD_RATES = 8;

#ifndef GPS_DEFAULT_BAUDRATE
    #define GPS_DEFAULT_BAUDRATE 9600
#endif

GPS::GPS(int rxPin, int txPin) : gpsSerial(rxPin, txPin)
{
    _rxPin = rxPin;
    _txPin = txPin;
    device.get_device_state()->gpsHz = 2;
}

void GPS::begin()
{
    Serial.println("[GPS] 开始初始化 txPin:" + String(_txPin) + " rxPin:" + String(_rxPin));
    // delay(1000);设置串口波特率
    gpsSerial.begin(GPS_DEFAULT_BAUDRATE);
    _currentBaudRate = GPS_DEFAULT_BAUDRATE;
#ifdef GPS_BAUDRATE
    if (!setBaudRate(GPS_BAUDRATE)) {
        Serial.println("[GPS] 设置波特率失败");
    }else {
        Serial.println("[GPS] 设置波特率成功,波特率:" + String(GPS_BAUDRATE));
    }
#endif

    // 设置频率
    if (!setHz(device.get_device_state()->gpsHz)) {
        Serial.println("[GPS] 设置频率失败");
    }else {
        Serial.println("[GPS] 设置频率成功,频率:" + String(device.get_device_state()->gpsHz));
    }

    // 设置双模 ，1北斗，2GPS，3双模
    if (!buildModeCmd(3)) {
        Serial.println("[GPS] 设置双模失败");
    }else {
        Serial.println("[GPS] 设置双模成功");
    }
}

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

void GPS::printRawData()
{
    // 打印缓冲区中的数据
    while (gpsSerial.available())
    {
        char c = gpsSerial.read();
        Serial.print(c);
    }
}

/**
 * 切换频率 在 1，2，5，10 Hz 之间切换
 * @return 切换后的频率
 */
int GPS::changeHz()
{
    // 计算下一个目标频率
    int nextHz = device.get_device_state()->gpsHz == 1 ? 2 : device.get_device_state()->gpsHz == 2 ? 5 : device.get_device_state()->gpsHz == 5 ? 10 : 1;
    
    // 尝试设置新频率
    if (setHz(nextHz)) {
        Serial.println("[GPS] 频率切换成功: " + String(nextHz) + "Hz");
        device.get_device_state()->gpsHz = nextHz;  // 只有在设置成功时才更新频率值
    } else {
        Serial.println("[GPS] 频率切换失败，保持当前频率: " + String(device.get_device_state()->gpsHz) + "Hz");
    }
    
    return device.get_device_state()->gpsHz;  // 返回实际的频率值
}

/**
 * NMEA 语法设置波特率
 * @param baudRate 目标波特率 (支持 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600)
 * @return bool 设置是否成功
 */
bool GPS::setBaudRate(int baudRate)
{
    String cmd = buildBaudrateCmd(baudRate);
    if (sendGpsCommand(cmd, 3, 100)) {
        _currentBaudRate = baudRate;
        return true;
    }
    return false;
}

/**
 * NMEA 语法设置频率
 * @param hz 目标频率 (支持 1, 2, 5, 10 Hz)
 * @return bool 设置是否成功
 */
bool GPS::setHz(int hz)
{
    String cmd = buildUpdateRateCmd(hz);
    return sendGpsCommand(cmd, 3, 100);
}

// GPS类的实现
String GPS::calculateChecksum(const String& cmd) {
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

String GPS::buildBaudrateCmd(int baudRate) {
    String cmd;
    switch (baudRate) {
        case 4800:   cmd = "$PCAS01,0"; break;
        case 9600:   cmd = "$PCAS01,1"; break;
        case 19200:  cmd = "$PCAS01,2"; break;
        case 38400:  cmd = "$PCAS01,3"; break;
        case 57600:  cmd = "$PCAS01,4"; break;
        case 115200: cmd = "$PCAS01,5"; break;
        case 230400: cmd = "$PCAS01,6"; break;
        case 460800: cmd = "$PCAS01,7"; break;
        case 921600: cmd = "$PCAS01,8"; break;
        default:     return ""; // 不支持的波特率返回空字符串
    }
    
    // 计算并添加校验和
    cmd += "*" + calculateChecksum(cmd);
    return cmd + "\r\n";
}

String GPS::buildModeCmd(int mode) {
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

String GPS::buildUpdateRateCmd(int ms) {
    if (ms <= 0) return "";
    
    String cmd = "$PCAS02," + String(ms);
    
    // 计算并添加校验和
    cmd += "*" + calculateChecksum(cmd);
    return cmd + "\r\n";
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

/*
* 依据卫星数量自动调节频率，20颗星以上10Hz，10-20颗星5Hz，10颗星以下2Hz，
* @param satellites 卫星数量
* @return 调节后的频率
*/
int GPS::autoAdjustHz(uint8_t satellites) {
    const int hz = satellites > 20 ? 10 : satellites > 10 ? 5 : 2;

    if (hz == device.get_device_state()->gpsHz) {
        // 符合预期直接返回
        return hz;
    }

    if (setHz(hz)) {
        Serial.println("[GPS] 自动调节频率成功: " + String(hz) + "Hz");
        device.get_device_state()->gpsHz = hz;
        return hz;
    } else {
        Serial.println("[GPS] 自动调节频率失败，保持当前频率: " + String(device.get_device_state()->gpsHz) + "Hz");
        return device.get_device_state()->gpsHz;
    }
}

/*
外部按键切换波特率
*/
int GPS::changeBaudRate() {
    // 获取当前波特率，并切换到下一个波特率
    int currentBaudRate = _currentBaudRate;
    int nextBaudRate = currentBaudRate == 9600 ? 19200 : currentBaudRate == 19200 ? 38400 : currentBaudRate == 38400 ? 57600 : currentBaudRate == 57600 ? 115200 : currentBaudRate == 115200 ? 230400 : currentBaudRate == 230400 ? 460800 : currentBaudRate == 460800 ? 921600 : 9600;
    if (setBaudRate(nextBaudRate)) {
        _currentBaudRate = nextBaudRate;
        return nextBaudRate;
    }
    return currentBaudRate;
}
