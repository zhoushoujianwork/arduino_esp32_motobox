#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>

typedef struct
{
    int battery;
    bool bleConnected;
    bool gpsReady;
    int gpsHz = 1; // 0-10HZ
    bool imuReady;
} device_state_t;

class Device
{
public:
    Device();
    void set_ble_connected(bool connected);
    void set_gps_ready(bool ready);
    void set_imu_ready(bool ready);
    bool get_ble_connected();
    void print_device_info();

    void set_battery(uint8_t battery);
    int get_battery();

    device_state_t *get_device_state();

private:
    device_state_t device_state;
};

extern Device device;

#endif