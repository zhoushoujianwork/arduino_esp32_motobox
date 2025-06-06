#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include "gps/GPS.h"
#include "qmi8658/IMU.h"
#include "wifi/WifiManager.h"
#include "device.h"
#include "power/PowerManager.h"

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
    String mqtt_topic_command;
    bool reconnect();

public:
    MQTT(const char *server, int port, const char *user, const char *password);
    void loop();

    // 发送GPS数据
    void publishGPS(gps_data_t gps_data);
    // 发送IMU数据
    void publishIMU(imu_data_t imu_data);
    // 发送设备信息
    void publishDeviceInfo(device_state_t device_state);

    void subscribeCommand(); // 订阅命令
    void handleCommand(char* topic, byte* payload, unsigned int length); // 处理命令
};

#endif