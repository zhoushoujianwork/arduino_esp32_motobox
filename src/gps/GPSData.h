/*
 * GPS数据结构定义
 * 统一外部GPS和GSM GNSS的数据格式
 */

#ifndef GPS_DATA_H
#define GPS_DATA_H

#include <Arduino.h>

typedef struct {
    // 基础状态
    bool valid;           // 定位是否有效
    
    // 时间信息
    uint16_t year;        // 年份
    uint8_t month;        // 月份 (1-12)
    uint8_t day;          // 日期 (1-31)
    uint8_t hour;         // 小时 (0-23)
    uint8_t minute;       // 分钟 (0-59)
    uint8_t second;       // 秒钟 (0-59)
    uint8_t centisecond;  // 百分之一秒 (0-99)

    // 位置信息
    double latitude;      // 纬度 (-90°~90°)
    double longitude;     // 经度 (-180°~180°)
    double altitude;      // 海拔高度，单位：米

    // 精度信息
    double hdop;          // 水平精度因子 (0.5最佳~50最差)
    double pdop;          // 位置精度因子
    double vdop;          // 垂直精度因子
    
    // 运动信息
    double speed;         // 速度，单位：千米/小时
    double heading;       // 航向角，单位：度 (0°~360°)
    
    // 卫星信息
    uint8_t satellites;   // 可见卫星数量
    uint8_t satellites_used; // 用于定位的卫星数量
    
    // 配置信息
    uint8_t gpsHz;        // GPS更新频率
    String gpsType;       // GPS类型标识 ("External", "GSM_GNSS")
    
    // 信号质量
    double cn0_max;       // 最大载噪比
    double hpa;           // 水平精度评估
    double vpa;           // 垂直精度评估
} gps_data_t;

// 全局GPS数据实例
extern gps_data_t gps_data;

// 数据转换和工具函数
String gps_data_to_json(const gps_data_t& data);
void reset_gps_data(gps_data_t& data);
bool is_gps_data_valid(const gps_data_t& data);

#endif // GPS_DATA_H 