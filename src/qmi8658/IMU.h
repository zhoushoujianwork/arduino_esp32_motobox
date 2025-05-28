#ifndef IMU_H
#define IMU_H

#include <Wire.h>
#include <SPI.h>
#include "SensorQMI8658.hpp"
#include "device.h"

#define ALPHA 0.98 // 互补滤波的系数，范围在0到1之间
#define dt 0.01    // 时间间隔，单位是秒（假设采样率为100Hz）

// 运动检测相关参数
// 运动检测相关参数
#define MOTION_DETECTION_THRESHOLD_DEFAULT 0.1   // 增加到 0.1g
#define MOTION_DETECTION_WINDOW_DEFAULT 32       // 增加窗口大小到32
#define MOTION_DETECTION_DEBOUNCE_MS 200        // 增加去抖时间到200ms

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
    bool enableMotionDetection(int intPin, float threshold = MOTION_DETECTION_THRESHOLD_DEFAULT);
    void disableMotionDetection();
    bool configureForDeepSleep();
    bool restoreFromDeepSleep();
    bool isMotionDetected();
    bool checkWakeOnMotionEvent();  // 新增：检查WakeOnMotion事件
    
    // 电源管理方法
    void setAccelPowerMode(uint8_t mode);  // 0=低功耗，1=正常，2=高性能
    void setGyroEnabled(bool enabled);
    
private:
    int sda;
    int scl;
    int motionIntPin;           // 运动检测中断引脚
    float motionThreshold;      // 运动检测阈值
    bool motionDetectionEnabled;// 运动检测是否启用
    SensorQMI8658 qmi;
    
    // 配置运动检测参数
    void configureMotionDetection(float threshold);
};

extern IMU imu;

#endif

// pitch 90 0   -90
