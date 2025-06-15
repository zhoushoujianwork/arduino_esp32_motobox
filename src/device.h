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
#include "qmi8658/IMU.h"
#include "power/PowerManager.h"
#include "ble/ble_client.h"
#include "ble/ble_server.h"
#include "compass/Compass.h"
#include "gps/GPS.h"
#include "bat/BAT.h"
#include "net/MqttManager.h"



typedef struct
{
    String device_id; // 设备ID
    String device_firmware_version; // 固件版本
    String device_hardware_version; // 硬件版本
    int sleep_time; // 休眠时间 单位：秒
    int battery_voltage;
    int battery_percentage;
    bool wifiConnected; // WiFi连接状态
    bool bleConnected; // BLE连接状态
    bool gpsReady; // GPS准备状态
    bool imuReady; // IMU准备状态
    bool compassReady;  // 罗盘准备状态
    bool gsmReady; // GSM准备状态
} device_state_t;

extern device_state_t device_state;

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
    
private:
    
};

extern Device device;

#endif
