#include "device.h"

Device::Device()
{
    device_state.bleConnected = false;
    device_state.battery = 1;
    device_state.gpsReady = false;
    device_state.imuReady = false;
}

void Device::print_device_info()
{
    Serial.print("Device Info:");
    Serial.printf("BLE Connected: %d\t", device_state.bleConnected);
    Serial.printf("Battery: %d\t", device_state.battery);
    Serial.printf("GPS Ready: %d\t", device_state.gpsReady);
    Serial.printf("GPS Hz: %d\t", device_state.gpsHz);
    Serial.printf("IMU Ready: %d\n", device_state.imuReady);
}

void Device::set_ble_connected(bool connected)
{
    device_state.bleConnected = connected;
}

void Device::set_gps_ready(bool ready)
{
    device_state.gpsReady = ready;
}

void Device::set_imu_ready(bool ready)
{
    device_state.imuReady = ready;
}

bool Device::get_ble_connected()
{
    return device_state.bleConnected;
}

void Device::set_battery(uint8_t battery)
{
    this->device_state.battery = battery;
}

int Device::get_battery()
{
    return device_state.battery;
}

device_state_t *Device::get_device_state()
{
    return &device_state;
}