#include "GPS.h"

// \r\n 是回车换行符的组合
// \r (CR, Carriage Return) - 回车，将光标移动到行首
// \n (LF, Line Feed) - 换行，将光标移动到下一行
// 在串行通信中,\r\n的组合是标准的行结束符
// NMEA协议规定每条GPS语句都必须以\r\n结尾

const char *UBX_CFG_PRT_4800 = "$PCAS01,0*1C\r\n";  
const char *UBX_CFG_PRT_9600 = "$PCAS01,1*1D\r\n";
const char *UBX_CFG_PRT_19200 = "$PCAS01,3*1B\r\n";
const char *UBX_CFG_PRT_38400 = "$PCAS01,2*1A\r\n";
const char *UBX_CFG_PRT_57600 = "$PCAS01,4*1E\r\n";
const char *UBX_CFG_PRT_115200 = "$PCAS01,5*19\r\n";

const char *UBX_CFG_RATE_1HZ  = "$PCAS02,1000*2E\r\n";  // 1000ms
const char *UBX_CFG_RATE_2HZ  = "$PCAS02,500*1A\r\n";   // 500ms  
const char *UBX_CFG_RATE_5HZ  = "$PCAS02,200*1D\r\n";   // 200ms
const char *UBX_CFG_RATE_10HZ = "$PCAS02,100*1E\r\n";   // 100ms

const char *UBX_CFG_MSG_GGA_OFF = "$PCAS04,0*1D\r\n";  // 关闭GGA
const char *UBX_CFG_MSG_GSA_OFF = "$PCAS04,1*1C\r\n";  // 关闭GSA  
const char *UBX_CFG_MSG_GSV_OFF = "$PCAS04,2*1F\r\n";  // 关闭GSV
const char *UBX_CFG_MSG_RMC_ON  = "$PCAS04,4,1*21\r\n"; // 开启RMC
const char *UBX_CFG_MSG_ZDA_ON = "$PCAS04,6,1*21\r\n"; // 开启ZDA

GPS::GPS(int rxPin, int txPin) : gpsSerial(rxPin, txPin)
{
}

void GPS::begin()
{
    Serial.println(TinyGPSPlus::libraryVersion());
    gpsSerial.begin(9600);  // 初始默认波特率

    // 优化设备信息查询
    gpsSerial.print("$PCAS06,1*1A\r\n");
    delay(100);
    while(gpsSerial.available()) {
        Serial.write(gpsSerial.read());
    }

    // 切换到 115200
    gpsSerial.begin(9600);
    delay(100);
/*
// 切换到GPS+北斗
$PCAS04,3*1A\r\n
// 切换到单GPS
$PCAS04,1*18\r\n
// 切换到单北斗
$PCAS04,2*1B\r\n
*/
    // 切换到北斗
    // gpsSerial.print("$PCAS04,2*1B\r\n");
    // delay(100);

    // 切换到GPS
    // gpsSerial.print("$PCAS04,1*18\r\n");
    // delay(100);

    // 切换到北斗+GPS
    gpsSerial.print("$PCAS04,3*1A\r\n");
    delay(100);

    
    // $GPGSV 卫星信息
    // $GPRMC 位置信息
    // $GPGGA 位置信息
    // $GPGSA 卫星信息
    // $GPVTG 速度信息
    // $GPZDA 时间信息

    // 增强NMEA输出过滤（只保留RMC）
    gpsSerial.print(UBX_CFG_MSG_GGA_OFF); // 关闭GGA 保留 RMC
    gpsSerial.print(UBX_CFG_MSG_RMC_ON);  

    // 开启时间
    gpsSerial.print(UBX_CFG_MSG_ZDA_ON);
    // 关闭GSV
    gpsSerial.print(UBX_CFG_MSG_GSV_OFF);
    
    delay(150);  // 增加等待时间确保配置生效

    setGpsHz(1); // 目前 1HZ 最稳定，其他频率还要调优

}

// 新增响应打印函数
void GPS::printGpsResponse(const char* context)
{
    Serial.print(context);
    Serial.println("响应:");
    while(gpsSerial.available()) {
        Serial.write(gpsSerial.read());
    }
}

// 优化后的频率设置函数
bool GPS::setGpsHz(int hz)
{
    const char *cmd = nullptr;
    switch (hz) {
        case 1:  cmd = UBX_CFG_RATE_1HZ;  break;
        case 2:  cmd = UBX_CFG_RATE_2HZ;  break;
        case 5:  cmd = UBX_CFG_RATE_5HZ;  break;
        case 10: cmd = UBX_CFG_RATE_10HZ; break;
        default: return false;
    }
    
    gpsSerial.print(cmd);
    delay(100);
    return true;
}

// 优化波特率设置函数
bool GPS::setGpsBaudRate(int baudRate)
{
    const char* configCmd = nullptr;
    switch (baudRate) {
        case 115200: configCmd = UBX_CFG_PRT_115200; break;
        case 57600:  configCmd = UBX_CFG_PRT_57600;  break;
        case 38400:  configCmd = UBX_CFG_PRT_38400;  break;
        case 19200:  configCmd = UBX_CFG_PRT_19200;  break;
        case 9600:   configCmd = UBX_CFG_PRT_9600;   break;
        case 4800:   configCmd = UBX_CFG_PRT_4800;   break;
        default:     return false;
    }
    
    gpsSerial.print(configCmd);
    delay(50);  // 短暂等待命令发送
    
    // 验证配置是否生效
    gpsSerial.begin(baudRate);
    if(gpsSerial.available()) {
        printGpsResponse("波特率设置");
        return true;
    }
    Serial.printf("波特率%d设置失败\n", baudRate);
    return false;
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
            
            // if (gps.speed.isUpdated())
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