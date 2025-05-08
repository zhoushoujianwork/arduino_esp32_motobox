#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include <WiFi.h>
#include "device.h"
#include "qmi8658/IMU.h"
#include "gps/GPS.h"
#include "tft/TFT.h"

// 低功耗模式状态枚举
enum PowerState {
    POWER_STATE_NORMAL,           // 正常工作状态
    POWER_STATE_COUNTDOWN,        // 倒计时状态
    POWER_STATE_PREPARING_SLEEP   // 正在准备进入睡眠状态
};

class PowerManager {
public:
    PowerManager();
    void begin();
    void loop();
    void enterLowPowerMode();
    void configureWakeupSources();
    bool setupIMUWakeupSource(int intPin, float threshold = 0.05);
    bool isDeviceIdle();
    
    // 新增：打断低功耗模式进入过程
    void interruptLowPowerMode();
    
    // 新增：获取当前电源状态
    PowerState getPowerState() { return powerState; }

    // 检查休眠功能是否启用（由编译时配置决定）
    bool isSleepEnabled() { return sleepEnabled; }

private:
    unsigned long lastMotionTime; // 最后一次检测到运动的时间
    unsigned long idleThreshold;  // 进入低功耗模式的空闲时间阈值（毫秒）
    float motionThreshold;        // 运动检测阈值
    PowerState powerState;        // 当前电源状态
    bool interruptRequested;      // 是否请求打断低功耗模式
    RTC_DATA_ATTR static bool sleepEnabled;  // 是否启用休眠功能（RTC内存，由编译时配置决定）
    
    bool detectMotion();          // 检测设备是否在运动
    void disablePeripherals();    // 关闭外设
};

extern PowerManager powerManager;

#endif // POWER_MANAGER_H 