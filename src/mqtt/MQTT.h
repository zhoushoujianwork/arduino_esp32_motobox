#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include "gps/GPS.h"
#include "qmi8658/IMU.h"

class MQTT
{
private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    const char *mqtt_server;
    const int mqtt_port;
    const char *mqtt_user;
    const char *mqtt_password;
    const char *client_id;

    void reconnect();

public:
    MQTT(const char *server, int port, const char *user, const char *password, const char *clientId);
    void begin();
    void loop();

    // 发送GPS数据
    void publishGPS(gps_data_t gps_data);
    // 发送IMU数据
    void publishIMU(imu_data_t imu_data);
};

#endif