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
    // 先尝试以115200波特率通信
    gpsSerial.begin(115200);
    delay(200);

    // 检查是否能收到数据
    if (!checkGpsResponse())
    {
        // 如果115200无响应，假设是9600波特率
        gpsSerial.begin(9600);
        delay(200);

        // 发送切换到115200波特率的命令
        gpsSerial.print(UBX_CFG_PRT_115200);
        delay(100);

        // 重新以115200波特率初始化
        gpsSerial.begin(115200);
        delay(200);
    }

    setGpsHz(2);
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
                gps_data.year = gps.date.year();
                gps_data.month = gps.date.month();
                gps_data.day = gps.date.day();
                gps_data.hour = gps.time.hour();
                gps_data.minute = gps.time.minute();
                gps_data.second = gps.time.second();
                gps_data.centisecond = gps.time.centisecond();
                gps_data.latitude = gps.location.lat();
                gps_data.longitude = gps.location.lng();
                gps_data.altitude = gps.altitude.meters();
                gps_data.speed = gps.speed.kmph();
                gps_data.heading = gps.course.deg();
                gps_data.satellites = gps.satellites.value();
            }
            // gps数量超过3颗星，则认为gpsReady为true
            if (gps_data.satellites > 3)
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

// 打印GPS数据
void GPS::printGpsData()
{
    char timeStr[30];
    sprintf(timeStr, "%04d-%02d-%02d %02d:%02d:%02d",
            gps_data.year, gps_data.month, gps_data.day,
            gps_data.hour, gps_data.minute, gps_data.second);

    Serial.println("gps_data: " + String(timeStr) + ", " +
                   String(gps_data.latitude) + ", " +
                   String(gps_data.longitude) + ", " +
                   String(gps_data.altitude) + ", " +
                   String(gps_data.speed) + ", " +
                   String(gps_data.heading) + ", " +
                   String(gps_data.satellites));
}
gps_data_t *GPS::get_gps_data()
{
    return &gps_data;
}

void GPS::resetGps()
{
    device.get_device_state()->gpsReady = false;
    newLocation = false;
    gps_data.year = 2025;
    gps_data.month = 1;
    gps_data.day = 22;
    gps_data.hour = 15;
    gps_data.minute = 41;
    gps_data.second = 00;
    gps_data.centisecond = 0;
    gps_data.latitude = 39;
    gps_data.longitude = 116;
    gps_data.altitude = random(100);
    gps_data.speed = random(300);     // 生成 0-299 的随机数
    gps_data.heading = random(360);   // 生成 0-359 的随机数
    gps_data.satellites = random(10); // 生成 0-9 的随机数
}
