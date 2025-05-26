#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>
#include "esp_system.h"
#include <ArduinoJson.h>
#include "compass/Compass.h"
#include "config.h"

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

typedef struct
{
    // 加速度计数据，单位：g
    float accel_x; // X轴加速度
    float accel_y; // Y轴加速度
    float accel_z; // Z轴加速度

    // 陀螺仪数据，单位：°/s
    float gyro_x; // X轴角速度
    float gyro_y; // Y轴角速度
    float gyro_z; // Z轴角速度

    // 姿态角，单位：度
    float roll;  // 横滚角
    float pitch; // 俯仰角
    float yaw;   // 偏航角

    float temperature; // 温度，单位：摄氏度
} imu_data_t;

typedef struct
{
    float x;          // X轴磁场强度
    float y;          // Y轴磁场强度
    float z;          // Z轴磁场强度
    float heading; // 航向角 0-360
} compass_data_t;

typedef struct
{
    // version
    String version;
    int battery_voltage;
    int battery_percentage;
    bool wifiConnected;
    bool bleConnected;
    bool gpsReady;
    bool imuReady;

    bool compassReady;  // 添加罗盘状态
} device_state_t;


class Device
{
public:
    Device();
    void init();
    void set_wifi_connected(bool connected);
    void set_ble_connected(bool connected);
    void set_gps_ready(bool ready);
    void set_imu_ready(bool ready);
    void print_device_info();
    void set_device_state(device_state_t *state);
    String get_device_id();
    String device_state_to_json();
    
    device_state_t *get_device_state();

    // gps
    gps_data_t *get_gps_data();
    void set_gps_data(gps_data_t *data);
    float getTotalDistanceKm();
    String gps_data_to_json();
    void printGpsData();

    // imu
    imu_data_t *get_imu_data();
    void set_imu_data(imu_data_t *data);
    String imu_data_to_json();
    void printImuData();

    // 罗盘相关方法
    compass_data_t *get_compass_data();
    void set_compass_data(compass_data_t *data);
    String compass_data_to_json();
    void printCompassData();

private:
    device_state_t device_state;
    gps_data_t gps_data;
    imu_data_t imu_data;
    compass_data_t compass_data;  // 添加罗盘数据成员
    float totalDistanceKm;
    long lastDistanceTime;
};

extern Device device;

#endif

// 添加IMU中断引脚定义
#ifndef IMU_INT_PIN
#define IMU_INT_PIN 4  // 默认使用GPIO4作为IMU中断引脚，请根据实际硬件修改
#endif
