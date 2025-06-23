/*
 * 外部GPS模块驱动
 * 基于TinyGPS++库实现
 */

#ifndef GPS_H
#define GPS_H

#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include "GPSData.h"
#include "config.h"

class GPS
{
public:
    GPS(int rxPin, int txPin);
    void begin();
    void loop();
    void printRawData();
    int changeHz();
    bool setBaudRate(int baudRate);
    bool setHz(int hz);
    // 依据卫星数量自动调节频率
    int autoAdjustHz(uint8_t satellites);
    int changeBaudRate();
    /**
     * @brief 打印GPS数据
     */
    void printGpsData();

    double getTotalDistanceKm() const { return totalDistanceKm; }

    void setDebug(bool debug) { _debug = debug; }

private:
    SoftwareSerial gpsSerial;
    TinyGPSPlus gps;  // TinyGPS++ 对象
    int _rxPin;
    int _txPin;
    int _currentBaudRate;
    bool _debug;
    bool _foundBaudRate; // 是否开启自动波特率检测
    unsigned long _lastDebugPrintTime;
    void debugPrint(const String& message);

    double totalDistanceKm;
    unsigned long lastDistanceTime;
    unsigned long lastAutoAdjustHzTime;

    void loopDistance();
    void loopAutoAdjustHz();
    
    // 构建波特率设置命令
    static String buildBaudrateCmd(int baudRate);
    // 构建工作模式设置命令
    static String buildModeCmd(int mode);
    // 构建更新率设置命令
    static String buildUpdateRateCmd(int ms);
    // 计算NMEA命令校验和
    static String calculateChecksum(const String& cmd);
    // 发送GPS命令
    bool sendGpsCommand(const String& cmd, int retries, int retryDelay);
    
    bool isValidGpsResponse();
};

#ifdef ENABLE_GPS
extern GPS gps;
#endif

#endif // GPS_H