#include <WiFi.h>
#include <PubSubClient.h>
#include "MQTT.h"
#include "config.h"
#include "wifi/WifiManager.h"

MQTT::MQTT(const char *server, int port, const char *user, const char *password)
    : mqtt_server(server), mqtt_port(port), mqtt_user(user), mqtt_password(password)
{
    mqttClient.setClient(wifiManager.wifiClient);
    mqttClient.setServer(mqtt_server, mqtt_port);

    // 修改主题构建方式
    mqtt_topic_gps = String("vehicle/v1/") + device.get_device_id() + "/gps/position";
    mqtt_topic_imu = String("vehicle/v1/") + device.get_device_id() + "/imu/gyro";
}

void MQTT::reconnect()
{
    Serial.println("正在连接MQTT服务器...");
    Serial.print("尝试连接到服务器: ");
    Serial.print(mqtt_server);
    Serial.print(":");
    Serial.println(mqtt_port);

    // 检查服务器端口是否通
    if (wifiManager.wifiClient.connect(mqtt_server, mqtt_port))
    {
        Serial.println("服务器端口连接成功,开始连接MQTT服务器...");
        Serial.print("mqtt_user: ");
        Serial.println(mqtt_user);
        Serial.print("mqtt_password: ");
        Serial.println(mqtt_password);

        if (mqttClient.connect(device.get_device_id().c_str(), mqtt_user, mqtt_password))
        {
            Serial.println("MQTT连接成功");
        }
        else
        {
            Serial.print("连接失败，错误码=");
            Serial.print(mqttClient.state());
            // 释放mqttClient
            mqttClient.disconnect();
        }
    }
    else
    {
        Serial.println("无法连接到服务器，端口可能不通");
    }
}

void MQTT::loop()
{
    if (!mqttClient.connected())
    {
        if (wifiManager.isConnected())
            reconnect();
    }
    else
    {
        if (!mqttClient.loop())
        {
            Serial.println("MQTT循环失败");
        }
    }
}

void MQTT::publishGPS(gps_data_t gps_data)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        if (!mqttClient.publish(mqtt_topic_gps.c_str(), gps_data_to_json(gps_data).c_str()))
        {
            Serial.println("GPS数据发布失败");
        }
    }
}

void MQTT::publishIMU(imu_data_t imu_data)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        if (!mqttClient.publish(mqtt_topic_imu.c_str(), imu_data_to_json(imu_data).c_str()))
        {
            Serial.println("IMU数据发布失败");
        }
    }
}