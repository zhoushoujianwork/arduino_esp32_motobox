#ifndef GPS_H
#define GPS_H

#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include "device.h"
#include "config.h"
#include <ArduinoJson.h>

// 将gps_data_t结构体转换为JSON字符串
String gps_data_to_json(gps_data_t gps_data);

class GPS
{
public:
    GPS(int rxPin, int txPin);
    void begin();
    void loop();
    bool setGpsHz(int hz);
    // 打印原始数据
    void printRawData();

private:
    // 检查 GPS 天线 状态
    bool checkGpsStatus();
    bool setGpsBaudRate(int baudRate);
    SoftwareSerial gpsSerial;
    TinyGPSPlus gps;
    int txmsgCount;
    void setMode1();
    void setMode2();
    void configGps(const char *configCmd);
    int _rxPin;
    int _txPin;
};

extern GPS gps;

#endif // GPS_H