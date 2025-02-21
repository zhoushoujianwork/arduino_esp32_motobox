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

GPS::GPS(int rxPin, int txPin) : gpsSerial(rxPin, txPin)
{
    newLocation = false;
}

void GPS::begin()
{
    Serial.println(TinyGPSPlus::libraryVersion());
    gpsSerial.begin(9600);  // 初始默认波特率
    
    // 明确波特率设置流程
    if(!setGpsBaudRate(9600)) { // 确保使用目标波特率
        Serial.println("GPS初始化失败");
        return;
    }
    
    // 优化设备信息查询
    gpsSerial.print("$PCAS06,1*1A\r\n");
    if(checkGpsResponse()) {
        printGpsResponse("设备信息");
    } else {
        Serial.println("未收到设备响应");
    }
    
    setGpsHz(2);  // 初始设置为2Hz
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
    
    // 非阻塞式等待响应
    unsigned long start = millis();
    while(millis() - start < 2000) { // 2秒超时
        if(gpsSerial.available()) {
            printGpsResponse("频率设置");
            device.get_device_state()->gpsHz = hz;
            return true;
        }
        delay(50);
    }
    Serial.println("频率设置未收到确认");
    return false;
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
    if(checkGpsResponse()) {
        Serial.printf("波特率%d设置成功\n", baudRate);
        return true;
    }
    Serial.printf("波特率%d设置失败\n", baudRate);
    return false;
}

// 添加新的辅助函数来检查GPS模块响应
bool GPS::checkGpsResponse()
{
    unsigned long startTime = millis();
    while (millis() - startTime < 1000)
    { // 等待1秒
        if (gpsSerial.available())
        {
            return true;
        }
    }
    return false;
}

// 无延迟的 loop 才能准确拿到数据，必须要执行
void GPS::loop()
{
    while (gpsSerial.available() > 0)
    {
        if (gps.encode(gpsSerial.read()))
        {
            if (gps.location.isUpdated())
            {
                // 使用更高效的时间获取方式
                auto date = gps.date;
                auto time = gps.time;
                auto loc = gps.location;
                
                newLocation = true;
                device.get_gps_data()->year = date.year();
                device.get_gps_data()->month = date.month();
                device.get_gps_data()->day = date.day();
                device.get_gps_data()->hour = time.hour();
                device.get_gps_data()->minute = time.minute();
                device.get_gps_data()->second = time.second();
                device.get_gps_data()->centisecond = time.centisecond();
                device.get_gps_data()->latitude = loc.lat();
                device.get_gps_data()->longitude = loc.lng();
                device.get_gps_data()->altitude = gps.altitude.meters();
                device.get_gps_data()->speed = gps.speed.kmph();
                device.get_gps_data()->heading = gps.course.deg();
                device.get_gps_data()->satellites = gps.satellites.value();
            }
        }
        // 移除delay以提升响应速度
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