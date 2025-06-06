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
    void begin();
    void loop();
    bool requestLowPowerMode;
    void enterLowPowerMode();
    bool configureWakeupSources();
    bool setupIMUWakeupSource(int intPin, float threshold = 0.05);
    bool isDeviceIdle();
    
    // 新增：打断低功耗模式进入过程
    void interruptLowPowerMode();
    
    // 新增：获取当前电源状态
    PowerState getPowerState() { return powerState; }

    // 检查休眠功能是否启用（由编译时配置决定）
    bool isSleepEnabled() { return sleepEnabled; }

    // 新增：唤醒原因打印与处理
    void printWakeupReason();
    void checkWakeupCause();


    // ==== 参数集中定义 ====
    static constexpr unsigned long DEFAULT_IDLE_THRESHOLD = 30000; // debug 30 秒 ，5分钟
    static constexpr float DEFAULT_MOTION_THRESHOLD = 0.1;         // 加速度变化阈值
    unsigned long lastMotionTime; // 最后一次检测到运动的时间

private:
    unsigned long idleThreshold;  // 进入低功耗模式的空闲时间阈值（毫秒）
    float motionThreshold;        // 运动检测阈值
    PowerState powerState;        // 当前电源状态
    bool interruptRequested;      // 是否请求打断低功耗模式
    RTC_DATA_ATTR static bool sleepEnabled;  // 是否启用休眠功能（RTC内存，由编译时配置决定）
    
    // 新增：运动检测相关变量
    static const int SAMPLE_COUNT = 5;  // 运动检测采样次数
    float lastAccelMagnitude;    // 上次加速度幅值
    float accumulatedDelta;      // 累积变化量
    int sampleIndex;             // 采样索引
    
    bool detectMotion();          // 检测设备是否在运动
    void disablePeripherals();    // 关闭外设
    void handleWakeup();          // 处理唤醒事件
    void configurePowerDomains(); // 配置电源域
    void safeDisableWiFi();      // 安全关闭WiFi
};

extern PowerManager powerManager;

#endif // POWER_MANAGER_H 