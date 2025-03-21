#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include "gps/GPS.h"
#include "qmi8658/IMU.h"
#include "wifi/WifiManager.h"
#include "device.h"

class MQTT
{
private:
    PubSubClient mqttClient;
    const char *mqtt_server;
    const int mqtt_port;
    const char *mqtt_user;
    const char *mqtt_password;
    String mqtt_topic_gps;
    String mqtt_topic_imu;
    String mqtt_topic_device_info;
    bool reconnect();
    bool isEnabled_; // 用于控制MQTT功能是否启用

public:
    MQTT(const char *server, int port, const char *user, const char *password);
    void loop();
    
    // 启用或禁用MQTT
    void setEnabled(bool enabled) { isEnabled_ = enabled; }
    bool isEnabled() const { return isEnabled_; }

    // 发送GPS数据
    void publishGPS(gps_data_t gps_data);
    // 发送IMU数据
    void publishIMU(imu_data_t imu_data);
    // 发送设备信息
    void publishDeviceInfo(device_state_t device_state);
    // 通用发布方法
    bool publish(const String& topic, const String& payload);
};

#endif