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

GPS::GPS(int rxPin, int txPin) : gpsSerial(rxPin, txPin)
{
    // 初始化时记录引脚号，便于调试
    _rxPin = rxPin;
    _txPin = txPin;
}

void GPS::configGps(const char *configCmd)
{
    for (int i = 0; i < 3; i++) {
        gpsSerial.print(configCmd);
        delay(100);
    }
}

void GPS::begin()
{
    Serial.println(TinyGPSPlus::libraryVersion());
    Serial.printf("GPS使用引脚 RX:%d TX:%d\n", _rxPin, _txPin);
    
    // 初始默认波特率 9600 启动然后设置
    if (GPS_DEFAULT_BAUDRATE == 9600) {
        gpsSerial.begin(115200);  // 初始默认波特率
    } else {
        gpsSerial.begin(9600);  // 初始默认波特率
    }
    delay(200); // 增加延时确保GPS模块稳定
    
    // 尝试多次设置波特率，确保命令被接收
    if(setGpsBaudRate(GPS_DEFAULT_BAUDRATE)) {
        Serial.printf("GPS波特率设置为: %d\n", GPS_DEFAULT_BAUDRATE);
    } else {
        Serial.println("GPS波特率设置失败");
    }
    delay(200);

    setMode1();
    delay(200);
    setGpsHz(GPS_HZ);
    delay(200);
    Serial.println("GPS 初始化完成");
}

void GPS::setMode1()
{
    // 北斗 单模
    // configGps("$PCAS04,2*1B\r\n");
    // GPS 单模
    configGps("$PCAS04,1*18\r\n");
    // 1000ms 输出一次 RMC 消息
    // configGps("$PCAS03,0,0,0,0,1,1,1,0,0,0,,,0,0,,,,0*33\r\n");
}

void GPS::setMode2()
{
    // 双模式
    configGps("$PCAS04,3*1A\r\n");
    // configGps("$PCAS03,0,0,0,0,0,0,1,0,0,0,,,0,0,,,,0*33\r\n");
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
    const char *cmd = nullptr;
    switch (hz) {
        case 1:  cmd = "$PCAS02,1000*2E\r\n";  break;
        case 2:  cmd = "$PCAS02,500*1A\r\n";  break;
        case 5:  cmd = "$PCAS02,200*1D\r\n";  break;
        case 10: cmd = "$PCAS02,100*1E\r\n"; break;
        default: return false;
    }
    configGps(cmd);
    
    // 同时更新device中的gpsHz值
    device.get_device_state()->gpsHz = hz;
    
    return true;
}

// 优化波特率设置函数
bool GPS::setGpsBaudRate(int baudRate)
{
    const char* configCmd = nullptr;
    switch (baudRate) {
        case 115200: configCmd = "$PCAS01,5*19\r\n"; break;
        case 9600:   configCmd = "$PCAS01,1*1D\r\n"; break;
        default:     return false;
    }
    
    // 多次发送配置命令，确保GPS模块接收到
    for (int i = 0; i < 5; i++) {
        gpsSerial.print(configCmd);
        delay(100);
    }

    // 重新配置串口波特率
    gpsSerial.end();
    delay(100);
    gpsSerial.begin(baudRate);
    
    return true;
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