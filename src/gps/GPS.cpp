#include "GPS.h"

// \r\n 是回车换行符的组合
// \r (CR, Carriage Return) - 回车，将光标移动到行首
// \n (LF, Line Feed) - 换行，将光标移动到下一行
// 在串行通信中,\r\n的组合是标准的行结束符
// NMEA协议规定每条GPS语句都必须以\r\n结尾
const char *UBX_CFG_PRT_9600 = "$PCAS01,1*1D\r\n";
const char *UBX_CFG_PRT_19200 = "$PCAS01,3*1B\r\n";
const char *UBX_CFG_PRT_38400 = "$PCAS01,2*1A\r\n";
const char *UBX_CFG_PRT_57600 = "$PCAS01,4*1E\r\n";
const char *UBX_CFG_PRT_115200 = "$PCAS01,5*19\r\n";

const char *UBX_CFG_RATE_1HZ = "$PCAS02,1000*2E\r\n";
const char *UBX_CFG_RATE_2HZ = "$PCAS02,500*1A\r\n";
const char *UBX_CFG_RATE_5HZ = "$PCAS02,200*1D\r\n";
const char *UBX_CFG_RATE_10HZ = "$PCAS02,100*1E\r\n";

GPS::GPS(int rxPin, int txPin) : gpsSerial(rxPin, txPin)
{
    newLocation = false;
}

void GPS::begin()
{
    // 直接使用9600波特率初始化
    gpsSerial.begin(9600);
    delay(200);

    // 检查是否能收到数据
    if (!checkGpsResponse())
    {
        Serial.println("GPS 9600 无响应");
        while (true)
        {
            Serial.println("GPS 无响应, 请检查GPS模块连接");
            delay(1000);
        }
    }
    Serial.println("GPS 初始化完成");
    // setGpsHz(1);
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
                newLocation = true;
                device.get_gps_data()->year = gps.date.year();
                device.get_gps_data()->month = gps.date.month();
                device.get_gps_data()->day = gps.date.day();
                device.get_gps_data()->hour = gps.time.hour();
                device.get_gps_data()->minute = gps.time.minute();
                device.get_gps_data()->second = gps.time.second();
                device.get_gps_data()->centisecond = gps.time.centisecond();
                device.get_gps_data()->latitude = gps.location.lat();
                device.get_gps_data()->longitude = gps.location.lng();
                device.get_gps_data()->altitude = gps.altitude.meters();
                device.get_gps_data()->speed = gps.speed.kmph();
                device.get_gps_data()->heading = gps.course.deg();
                device.get_gps_data()->satellites = gps.satellites.value();

                // 新增里程计算逻辑
                if (lastUpdateTime != 0)
                {
                    unsigned long currentTime = millis();
                    float deltaTimeH = (currentTime - lastUpdateTime) / 3600000.0; // 转换为小时
                    totalDistanceKm += device.get_gps_data()->speed * deltaTimeH;                // 距离 = 速度 × 时间
                }
                lastUpdateTime = millis(); // 更新时间戳
            }
            // gps数量超过3颗星，则认为gpsReady为true
            if (device.get_gps_data()->satellites > 3)
                device.set_gps_ready(true);
            else
                device.set_gps_ready(false);
        }
    }
}

bool GPS::setGpsHz(int hz)
{
    const char *cmd = nullptr;

    switch (hz)
    {
    case 1:
        cmd = UBX_CFG_RATE_1HZ;
        break;
    case 2:
        cmd = UBX_CFG_RATE_2HZ;
        break;
    case 5:
        cmd = UBX_CFG_RATE_5HZ;
        break;
    case 10:
        cmd = UBX_CFG_RATE_10HZ;
        break;
    default:
        return false; // 不支持的频率
    }

    gpsSerial.print(cmd);
    delay(100); // 等待配置生效
    device.get_device_state()->gpsHz = hz;
    return true;
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