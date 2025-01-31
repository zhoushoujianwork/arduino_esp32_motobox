#ifndef GPS_H
#define GPS_H

#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include "device.h"

// NMEA0183 配置
// PCAS语句是由芯片供应商定义的NMEA专用语句

// 9600
extern const char *UBX_CFG_PRT_9600;
// 19200bps
extern const char *UBX_CFG_PRT_19200;
// 38400bps
extern const char *UBX_CFG_PRT_38400;
// 57600bps
extern const char *UBX_CFG_PRT_57600;
// 设置波特率为 115200
extern const char *UBX_CFG_PRT_115200;

// 设置 NMEA 更新速率为 1Hz
extern const char *UBX_CFG_RATE_1HZ;
// 设置 NMEA 更新速率为 2Hz
extern const char *UBX_CFG_RATE_2HZ;
// 设置 NMEA 更新速率为 5Hz
extern const char *UBX_CFG_RATE_5HZ;
// 设置 10Hz $PCAS02,100*1E
extern const char *UBX_CFG_RATE_10HZ;

// PCAS03
// 格式： $PCAS03,nGGA,nGLL,nGSA,nGSV,nRMC,nVTG,nZDA,nANT,nDHV,nLPS,res1,res2,nUTC,nGST,res3,res4,res5,nTIM*CS<CR><LF>
// GGA 接收机定位数据
// GLL 地理位置经纬度
// GSA 精度因子和有效卫星
// GSV 可见卫星
// RMC 推荐的最少专用导航数据
// VTG 地面速度和方位角
// GST 接收机伪距误差统计信息
// ZDA 日期和时间
// ANT 天线高度
// LPS 卫星系统闰秒修正信息
// DHV 接收机速度信息
extern const char *UBX_CFG_PRT_ALL;
// 只接受 RMC GGA 信息
extern const char *UBX_CFG_PRT_RMC;

/*
$PCAS04 设置模式

$PCAS04,3*1A 北斗和 GPS 双模
$PCAS04,1*18 单 GPS 工作模式
$PCAS04,2*1B 单北斗工作模式
*/

/*
设置 NMEA 协议类型选择。
$PCAS05,1*19
2 兼容 NMEA 4.1 以上版本
5 兼容中国交通运输信息中心的 BDS/GPS 双模协议，兼容 NMEA 2.3 以上版本，兼容NMEA4.0 协议
9 兼容单 GPS NMEA0183 协议，兼容 NMEA 2.2 版本
*/

/*
查询产品信息
$PCAS06,0*1B
*/

/*
设置重启
$PCAS10,0*1C 热启动
$PCAS10,1*1D 温启动
$PCAS10,2*1E 冷启动
$PCAS10,3*1F 出厂启动
*/

/*
$PCAS12,60*28 接收机进入待机模式，持续 60 秒后自动开机
*/

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

    // 运动信息
    double speed;       // 速度，单位：千米/小时
    double heading;     // 航向角，单位：度 (0°~360°)
    uint8_t satellites; // 可见卫星数量
} gps_data_t;

class GPS
{
public:
    GPS(int rxPin, int txPin);
    void begin();
    void loop();
    bool setGpsHz(int hz);
    int getGpsHz();
    void printRawData();
    void printGpsData();
    gps_data_t *get_gps_data();
    void resetGps();
    bool checkGpsResponse();

private:
    SoftwareSerial gpsSerial;
    TinyGPSPlus gps;
    bool newLocation;
    gps_data_t gps_data;
};

extern GPS gps;

#endif // GPSMODULE_H