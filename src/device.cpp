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

String Device::get_device_id()
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char device_id[13];
    snprintf(device_id, sizeof(device_id), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(device_id);
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