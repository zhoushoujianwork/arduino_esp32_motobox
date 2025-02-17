#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>
#include "esp_system.h"
#include <ArduinoJson.h>

typedef struct
{
    int battery_voltage;
    int battery_percentage;
    bool mqttConnected;
    bool wifiConnected;
    bool bleConnected;
    bool gpsReady;
    int gpsHz; // 0-10HZ
    bool imuReady;
} device_state_t;

class Device
{
public:
    Device();
    void set_mqtt_connected(bool connected);
    void set_wifi_connected(bool connected);
    void set_ble_connected(bool connected);
    void set_gps_ready(bool ready);
    void set_imu_ready(bool ready);
    bool get_mqtt_connected();
    bool get_wifi_connected();
    bool get_ble_connected();
    void print_device_info();
    String device_state_to_json();

    device_state_t *get_device_state();
    String get_device_id();

private:
    device_state_t device_state;
};

extern Device device;

#endif