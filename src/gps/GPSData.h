/*
 * GPS数据结构定义
 * 统一外部GPS和GSM GNSS的数据格式
 */

#ifndef GPS_DATA_H
#define GPS_DATA_H

#include <Arduino.h>

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

// 全局GPS数据实例
extern gps_data_t gps_data;

// 数据转换和工具函数
String gps_data_to_json(const gps_data_t& data);
void reset_gps_data(gps_data_t& data);
bool is_gps_data_valid(const gps_data_t& data);
void print_gps_data(const gps_data_t& data);

#endif // GPS_DATA_H 