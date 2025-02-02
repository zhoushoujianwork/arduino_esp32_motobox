#ifndef IMU_H
#define IMU_H

#include <Wire.h>
#include <SPI.h>
#include "SensorQMI8658.hpp"
#include "device.h"
#include <ArduinoJson.h>

#define ALPHA 0.98 // 互补滤波的系数，范围在0到1之间
#define dt 0.01    // 时间间隔，单位是秒（假设采样率为100Hz）

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

// 将imu_data_t结构体转换为JSON字符串
String imu_data_to_json(imu_data_t imu_data);

class IMU
{
public:
    IMU(int sda, int scl); // 默认I2C引脚
    void begin();
    void loop();
    float getRoll();
    float getPitch();
    bool isReady();
    imu_data_t *get_imu_data();
    void printImuData();

private:
    int sda;
    int scl;
    imu_data_t imu_data;
    SensorQMI8658 qmi;
};

extern IMU imu;

#endif
