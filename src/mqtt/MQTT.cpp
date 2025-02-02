#include <WiFi.h>
#include <PubSubClient.h>
#include "MQTT.h"
#include "config.h"

MQTT::MQTT(const char *server, int port, const char *user, const char *password, const char *clientId)
    : mqtt_server(server), mqtt_port(port), mqtt_user(user), mqtt_password(password), client_id(clientId)
{
    mqttClient.setClient(wifiClient);
    mqttClient.setServer(mqtt_server, mqtt_port);
}

void MQTT::begin()
{
    reconnect();
}

void MQTT::reconnect()
{
    while (!mqttClient.connected())
    {
        Serial.println("正在连接MQTT服务器...");
        if (mqttClient.connect(client_id, mqtt_user, mqtt_password))
        {
            Serial.println("MQTT连接成功");
        }
        else
        {
            Serial.print("连接失败，错误码=");
            Serial.print(mqttClient.state());
            Serial.println(" 5秒后重试...");
            delay(5000);
        }
    }
}

void MQTT::loop()
{
    if (!mqttClient.connected())
    {
        reconnect();
    }
    mqttClient.loop();
}

void MQTT::publishGPS(gps_data_t gps_data)
{
    mqttClient.publish(MQTT_TOPIC_GPS, gps_data_to_json(gps_data).c_str());
}

void MQTT::publishIMU(imu_data_t imu_data)
{
    mqttClient.publish(MQTT_TOPIC_IMU, imu_data_to_json(imu_data).c_str());
}