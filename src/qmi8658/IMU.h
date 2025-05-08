#ifndef IMU_H
#define IMU_H

#include <Wire.h>
#include <SPI.h>
#include "SensorQMI8658.hpp"
#include "device.h"

#define ALPHA 0.98 // 互补滤波的系数，范围在0到1之间
#define dt 0.01    // 时间间隔，单位是秒（假设采样率为100Hz）

class IMU
{
public:
    IMU(int sda, int scl, int motionIntPin = -1);
    void begin();
    void loop();
    // 运动检测中断标志和ISR
    static volatile bool motionInterruptFlag;
    static void IRAM_ATTR motionISR();
private:
    int sda;
    int scl;
    int motionIntPin;           // 运动检测中断引脚
    SensorQMI8658 qmi;
};

extern IMU imu;

#endif

// pitch 90 0   -90
