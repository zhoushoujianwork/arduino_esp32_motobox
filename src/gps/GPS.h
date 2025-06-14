#ifndef GPS_H
#define GPS_H

#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <ArduinoJson.h>
#include "device.h"
#include "config.h"
#include "ml370/MqttManager.h"


typedef struct
{
    // 时间信息
    uint16_t year;       // 年份
    uint8_t month;       // 月份 (1-12)
    uint8_t day;         // 日期 (1-31)
    uint8_t hour;        // 小时 (0-23)
    uint8_t minute;      // 分钟 (0-59)
    uint8_t second;      // 秒钟 (0-59)
    uint8_t centisecond; // 百分之一秒 (0-99)

    // 位置信息
    double latitude;  // 纬度 (-90°~90°)
    double longitude; // 经度 (-180°~180°)
    double altitude;  // 海拔高度，单位：米

/*
表示卫星几何分布对水平定位精度的影响
数值范围：0.5（最佳）~ 50（最差）
理想值 < 1.0，实际应用建议 < 2.0
*/
    double hdop; // 水平精度因子
    // 运动信息
    double speed;       // 速度，单位：千米/小时
    double heading;     // 航向角，单位：度 (0°~360°)
    uint8_t satellites; // 可见卫星数量
    
    uint8_t gpsHz;      // GPS更新频率
} gps_data_t;

extern gps_data_t gps_data;

String gps_data_to_json(const gps_data_t& data);


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

private:
    SoftwareSerial gpsSerial;
    TinyGPSPlus gps;  // TinyGPS++ 对象
    int _rxPin;
    int _txPin;
    int _currentBaudRate;

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
};
#ifdef ENABLE_GPS
extern GPS gps;
#endif


#endif // GPS_H