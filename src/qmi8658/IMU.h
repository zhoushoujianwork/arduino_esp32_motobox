#ifndef IMU_H
#define IMU_H


#include <Wire.h>
#include <SPI.h>
#include "SensorQMI8658.hpp"
#include "device.h"
#include "config.h"

#define ALPHA 0.98 // 互补滤波的系数，范围在0到1之间
#define dt 0.01    // 时间间隔，单位是秒（假设采样率为100Hz）

// 运动检测相关参数
#define MOTION_DETECTION_THRESHOLD_DEFAULT 0.0035   // 0.05 适合震动检测，, 静止的量级0.001~0.003
#define MOTION_DETECTION_WINDOW_DEFAULT 32       // 增加窗口大小到32
#define MOTION_DETECTION_DEBOUNCE_MS 200        // 增加去抖时间到200ms


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

extern imu_data_t imu_data;

String imu_data_to_json(imu_data_t& imu_data);

class IMU
{
public:
    IMU(int sda, int scl, int motionIntPin = -1);
    void begin();
    void loop();
    
    // 运动检测中断标志和ISR
    static volatile bool motionInterruptFlag;
    static void IRAM_ATTR motionISR();
    
    // 低功耗相关方法
    void disableMotionDetection();
    bool configureForDeepSleep();
    bool restoreFromDeepSleep();
    bool isMotionDetected();
    bool checkWakeOnMotionEvent();  // 新增：检查WakeOnMotion事件
    
    // 电源管理方法
    void setAccelPowerMode(uint8_t mode);  // 0=低功耗，1=正常，2=高性能
    void setGyroEnabled(bool enabled);

    /**
     * @brief 检测是否有运动
     * @return true: 检测到运动, false: 未检测到
     */
    bool detectMotion();

    /**
     * @brief 打印IMU数据
     */
    void printImuData();

private:
    int sda;
    int scl;
    bool _debug;
    int motionIntPin;           // 运动检测中断引脚
    float motionThreshold;      // 运动检测阈值
    bool motionDetectionEnabled;// 运动检测是否启用
    TwoWire& _wire; // 使用 Wire1 作为 I2C 总线
    SensorQMI8658 qmi;
    
    // 配置运动检测参数
    void configureMotionDetection(float threshold);

    // 运动检测相关变量
    float lastAccelMagnitude = 0;
    float accumulatedDelta = 0;
    int sampleIndex = 0;
    int sampleWindow = MOTION_DETECTION_WINDOW_DEFAULT;

    void debugPrint(const String& message);
    unsigned long _lastDebugPrintTime;

};

#ifdef ENABLE_IMU
extern IMU imu;
#endif

#endif

// pitch 90 0   -90
