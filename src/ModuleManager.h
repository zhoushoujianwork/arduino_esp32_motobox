#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include "Arduino.h"
#include "bat/BAT.h"
#include "ble/ble_server.h"
#include "ble/ble_client.h"
#include "btn/BTN.h"
#include "config.h"
#include "device.h"
#include "gps/GPS.h"
#include "led/LED.h"
#include "mqtt/MQTT.h"
#include "tft/TFT.h"
#include "qmi8658/IMU.h"
#include "compass/COMPASS.h"
#include "wifi/WifiManager.h"
#include "power/PowerManager.h"
#include "driver/gpio.h"

// 声明全局变量
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
extern GPS gps;
extern IMU imu;
extern TaskHandle_t gpsTaskHandle;
#endif

// 任务类型枚举
enum TaskType {
    TASK_WIFI,
    TASK_GPS,
    TASK_COMPASS,
    TASK_SYSTEM,
    TASK_DATA
};

class ModuleManager {
public:
    ModuleManager();
    
    // 初始化函数
    void begin();
    
    // 创建所有任务
    void createTasks();
    
    // 处理唤醒
    void handleWakeup();
    
    // 按钮事件处理
    void handleButtonEvents();
    
    // 获取设备实例
    Device* getDevice() { return &device; }
    
private:
    // 设备管理实例
    Device device;
    
    // 任务句柄
    TaskHandle_t wifiInitTaskHandle;
    
    // 时间记录变量
    unsigned long lastGpsPublishTime;
    unsigned long lastImuPublishTime;
    unsigned long lastBlePublishTime;
    unsigned long lastCompassPublishTime;
    
    // 设备实例声明
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    BTN button;
    MQTT mqtt;
#endif

#ifdef MODE_CLIENT
    BLEC bc;
#endif

#ifdef MODE_SERVER
    BLES bs;
#endif

    BAT bat;
    LED led;
    PowerManager powerManager;
    
#ifdef ENABLE_COMPASS
    COMPASS compass;
#endif

    // 打印唤醒原因
    void printWakeupReason();
    
    // 初始化所有硬件
    void initializeHardware();
    
    // 任务函数
    static void taskWifiInit(void *parameter);
    static void taskWifi(void *parameter);
    static void taskGps(void *parameter);
#ifdef ENABLE_COMPASS
    static void taskCompass(void *parameter);
#endif
    static void taskSystem(void *parameter);
    static void taskDataProcessing(void *parameter);
    
    // 各个任务的实际实现
    void wifiInitImpl();
    void wifiLoopImpl();
    void gpsLoopImpl();
#ifdef ENABLE_COMPASS
    void compassLoopImpl();
#endif
    void systemLoopImpl();
    void dataProcessingImpl();
    
    // 静态指针指向当前实例，用于任务函数访问非静态成员
    static ModuleManager* instance;
};

extern ModuleManager moduleManager;

#endif /* MODULE_MANAGER_H */ 