#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include <WiFi.h>
#include "device.h"
#include "qmi8658/IMU.h"
#include "gps/GPS.h"
#include "wifi/WifiManager.h"

// 低功耗模式状态枚举
enum PowerState {
    POWER_STATE_NORMAL,           // 正常工作状态
    POWER_STATE_COUNTDOWN,        // 倒计时状态
    POWER_STATE_PREPARING_SLEEP   // 正在准备进入睡眠状态
};

class PowerManager {
public:
    PowerManager();
    PowerState powerState;        // 当前电源状态
    void loop();
    bool requestLowPowerMode;
    void enterLowPowerMode();
    bool configureWakeupSources();
    bool isDeviceIdle();
    
    // 新增：获取/设置休眠时间（秒）
    void setSleepTime(unsigned long seconds);
    unsigned long getSleepTime() const;
    
    // 新增：获取当前电源状态
    PowerState getPowerState() { return powerState; }

    // 检查休眠功能是否启用（由编译时配置决定）
    bool isSleepEnabled() { return sleepEnabled; }

    // 新增：唤醒原因打印与处理
    void printWakeupReason();
    void checkWakeupCause();

     // 新增：打断低功耗模式进入过程
    void interruptLowPowerMode();



private:
    unsigned long idleThreshold;  // 进入低功耗模式的空闲时间阈值（毫秒）
    float motionThreshold;        // 运动检测阈值
    RTC_DATA_ATTR static bool sleepEnabled;  // 是否启用休眠功能（RTC内存，由编译时配置决定）
    
    // 新增：运动检测相关变量
    static const int SAMPLE_COUNT = 5;  // 运动检测采样次数
    float accumulatedDelta;      // 累积变化量
    
    void disablePeripherals();    // 关闭外设
    void handleWakeup();          // 处理唤醒事件
    void configurePowerDomains(); // 配置电源域

    unsigned long lastMotionTime; // 最后一次检测到运动的时间
    unsigned long sleepTimeSec;   // 休眠时间（秒）
   
};

extern PowerManager powerManager;

#endif // POWER_MANAGER_H 