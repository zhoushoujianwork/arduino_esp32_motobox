#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>
#ifdef ENABLE_GPS
#include "gps/GPS.h"
#endif
#ifdef ENABLE_IMU
#include "qmi8658/IMU.h"
#endif
#ifdef ENABLE_WIFI
#include "wifi/WifiManager.h"
#endif

#include "power/PowerManager.h"
#include "config.h"
#include <ArduinoJson.h>
#include "net/NetManager.h"

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
    Client* networkClient = nullptr; // 新增，当前底层 Client

    // 发布时间记录
    unsigned long lastGpsPublishTime = 0;
    unsigned long lastImuPublishTime = 0;
    unsigned long lastDeviceInfoPublishTime = 0;

    bool reconnect();

    void subscribeCommand(); // 订阅命令
    void handleCommand(char* topic, byte* payload, unsigned int length); // 处理命令

public:
    MQTT(const char *server, int port, const char *user, const char *password);
    void setClient(Client* client); // 新增，动态切换底层 Client
    void loop();
    void disconnect(); // 新增，断开MQTT连接，适配低功耗
};

extern MQTT mqtt;
#endif