#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>
#include "esp_system.h"
#include <ArduinoJson.h>
#include "compass/Compass.h"
#include "config.h"
#include "version.h"  // 包含版本信息头文件
#include "led/PWMLED.h"
#include "led/LED.h"
#include "imu/qmi8658.h"
#include "power/PowerManager.h"
#include "ble/ble_client.h"
#include "ble/ble_server.h"
#include "compass/Compass.h"
#include "bat/BAT.h"
// MQTT管理器已完全禁用
// #ifndef DISABLE_MQTT
// #include "net/MqttManager.h"
// #endif
#ifdef RTC_INT_PIN
#include "power/ExternalPower.h"
#endif
#include "led/LEDManager.h"

#ifdef ENABLE_SDCARD
#include "SD/SDManager.h"
#endif

#ifdef ENABLE_AUDIO
#include "audio/AudioManager.h"
#endif



typedef struct
{
    String device_id; // 设备ID
    String device_firmware_version; // 固件版本
    String device_hardware_version; // 硬件版本
    int sleep_time; // 休眠时间 单位：秒
    int led_mode; // LED模式 0:关闭 1:常亮 2:单闪 3:双闪 4:慢闪 5:快闪 6:呼吸 7:5秒闪烁
    int battery_voltage;
    int battery_percentage;
    bool is_charging;          // 新增：充电状态
    bool external_power;       // 新增：外部电源接入状态（车辆电门）
    bool wifiConnected; // WiFi连接状态
    bool bleConnected; // BLE连接状态
    bool imuReady; // IMU准备状态
    bool compassReady;  // 罗盘准备状态
    bool gsmReady; // GSM准备状态
    bool lbsReady; // LBS准备状态
    bool gnssReady; // GNSS准备状态
    
    // GPS/GNSS数据字段
    double latitude;    // 纬度
    double longitude;   // 经度
    int satellites;     // 卫星数量
    int signalStrength; // GSM信号强度
    
    bool sdCardReady; // SD卡准备状态
    uint64_t sdCardSizeMB; // SD卡大小(MB)
    uint64_t sdCardFreeMB; // SD卡剩余空间(MB)
    bool audioReady; // 音频系统准备状态
} device_state_t;

// 添加状态变化跟踪
typedef struct {
        bool battery_changed;      // 电池状态变化
        bool external_power_changed; // 外部电源状态变化
        bool wifi_changed;         // WiFi连接状态变化
        bool ble_changed;          // BLE连接状态变化
        bool gps_changed;          // GPS状态变化
        bool imu_changed;          // IMU状态变化
        bool compass_changed;      // 罗盘状态变化
        bool gsm_changed;          // GSM状态变化
        bool sleep_time_changed;   // 休眠时间变化
        bool led_mode_changed;     // LED模式变化
        bool sdcard_changed;       // SD卡状态变化
        bool audio_changed;        // 音频状态变化
} state_changes_t;

void update_device_state();

extern device_state_t device_state;
extern state_changes_t state_changes;

String device_state_to_json(device_state_t *state);

device_state_t *get_device_state();
void set_device_state(device_state_t *state);
void print_device_info();


class Device
{
public:
    Device();
    String get_device_id();

    // 硬件初始化
    void begin();
    
    // MQTT初始化，需要在网络就绪后调用
    bool initializeMQTT();
    
    // GSM初始化
    void initializeGSM();
    
    // 欢迎语音配置
    void setWelcomeVoiceType(int voiceType);
    void playWelcomeVoice();
    String getWelcomeVoiceInfo();
    
private:
};

extern Device device;

#endif
