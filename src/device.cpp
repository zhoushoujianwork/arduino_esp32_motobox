#include "device.h"

Device::Device()
{
}

void Device::init()
{
    device_state.wifiConnected = false;
    device_state.bleConnected = false;
    device_state.mqttConnected = false;
    device_state.battery_voltage = 0;
    device_state.battery_percentage = 0;
    device_state.gpsReady = false;
    device_state.imuReady = false;
    device_state.gpsHz = 1;
}

void Device::print_device_info()
{
    Serial.print("Device Info:");
    Serial.printf("MQTT Connected: %d\t", device_state.mqttConnected);
    Serial.printf("WiFi Connected: %d\t", device_state.wifiConnected);
    Serial.printf("BLE Connected: %d\t", device_state.bleConnected);
    Serial.printf("Battery Voltage: %d\t", device_state.battery_voltage);
    Serial.printf("Battery Percentage: %d\t", device_state.battery_percentage);
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

void Device::set_mqtt_connected(bool connected)
{
    if (device_state.mqttConnected != connected) {
        Serial.printf("MQTT状态变更: %d -> %d\n", device_state.mqttConnected, connected);
        device_state.mqttConnected = connected;
    }
}

void Device::set_wifi_connected(bool connected)
{
    if (device_state.wifiConnected != connected) {
        Serial.printf("WiFi状态变更: %d -> %d\n", device_state.wifiConnected, connected);
        device_state.wifiConnected = connected;
    }
}

void Device::set_ble_connected(bool connected)
{
    if (device_state.bleConnected != connected) {
        Serial.printf("BLE状态变更: %d -> %d\n", device_state.bleConnected, connected);
        device_state.bleConnected = connected;
    }
}

void Device::set_gps_ready(bool ready)
{
    if (device_state.gpsReady != ready) {
        Serial.printf("GPS状态变更: %d -> %d\n", device_state.gpsReady, ready);
        device_state.gpsReady = ready;
    }
}

void Device::set_imu_ready(bool ready)
{
    if (device_state.imuReady != ready) {
        Serial.printf("IMU状态变更: %d -> %d\n", device_state.imuReady, ready);
        device_state.imuReady = ready;
    }
}

bool Device::get_mqtt_connected()
{
    return device_state.mqttConnected;
}

bool Device::get_wifi_connected()
{
    return device_state.wifiConnected;
}

bool Device::get_ble_connected()
{
    return device_state.bleConnected;
}

device_state_t *Device::get_device_state()
{
    return &device_state;
}

void Device::set_device_state(device_state_t *state)
{
    device_state = *state;
}

String Device::device_state_to_json()
{
    StaticJsonDocument<200> doc;
    doc["mqttConnected"] = device_state.mqttConnected;
    doc["wifiConnected"] = device_state.wifiConnected;
    doc["bleConnected"] = device_state.bleConnected;
    doc["gpsReady"] = device_state.gpsReady;
    doc["gpsHz"] = device_state.gpsHz;
    doc["imuReady"] = device_state.imuReady;
    doc["batteryVoltage"] = device_state.battery_voltage;
    doc["batteryPercentage"] = device_state.battery_percentage;
    return doc.as<String>();
}