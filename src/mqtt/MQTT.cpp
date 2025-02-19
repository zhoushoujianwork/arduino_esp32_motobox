#include <WiFi.h>
#include <PubSubClient.h>
#include "MQTT.h"
#include "config.h"
#include "wifi/WifiManager.h"

MQTT::MQTT(const char *server, int port, const char *user, const char *password)
    : mqtt_server(server), mqtt_port(port), mqtt_user(user), mqtt_password(password)
{
    if (port == 8883)
        mqttClient.setClient(wifiManager.wifiClientSecure);
    else
        mqttClient.setClient(wifiManager.wifiClient);

    mqttClient.setServer(mqtt_server, mqtt_port);

    // 修改主题构建方式
    mqtt_topic_gps = MQTT_TOPIC_GPS;
    mqtt_topic_imu = MQTT_TOPIC_IMU;
    mqtt_topic_device_info = MQTT_TOPIC_DEVICE_INFO;
}

bool MQTT::reconnect()
{
    Serial.println("正在连接MQTT服务器...");
    Serial.print("尝试SSL连接到服务器: ");
    Serial.print(mqtt_server);
    Serial.print(":");
    Serial.println(mqtt_port);

    // 增加MQTT连接超时时间
    mqttClient.setSocketTimeout(15); // 设置15秒超时

    if (device.get_device_state()->wifiConnected && mqttClient.connect(device.get_device_id().c_str(), mqtt_user, mqtt_password))
    {
        return true;
    }
    else
    {
        Serial.print("连接失败，错误码=");
        Serial.print(mqttClient.state());
        Serial.println(" (提示:-4=连接超时,-2=连接被拒绝,-3=服务器不可达)");
        mqttClient.disconnect();
        return false;
    }
}

void MQTT::loop()
{
    if (!device.get_device_state()->wifiConnected)
    {
        Serial.println("WiFi未连接，MQTT连接失败");
        device.set_mqtt_connected(false);
        return;
    }

    if (!mqttClient.connected())
    {
        Serial.println("MQTT连接断开，尝试重新连接");
        if (!reconnect())
        {
            Serial.println("MQTT连接失败");
            device.set_mqtt_connected(false);
            return;
        }
        else
        {
            Serial.println("MQTT连接成功");
            device.set_mqtt_connected(true);
        }
    }

    if (!mqttClient.loop())
    {
        Serial.println("MQTT循环失败");
        device.set_mqtt_connected(false);
    }
}

void MQTT::publishGPS(gps_data_t gps_data)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        if (!mqttClient.publish(mqtt_topic_gps.c_str(), device.gps_data_to_json().c_str()))
        {
            Serial.println("GPS数据发布失败");
        }
    }
    else
    {
        // Serial.println("GPS数据发布失败,WiFi未连接");
    }
}

void MQTT::publishIMU(imu_data_t imu_data)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        if (!mqttClient.publish(mqtt_topic_imu.c_str(), device.imu_data_to_json().c_str()))
        {
            Serial.println("IMU数据发布失败");
        }
    }
    else
    {
        // Serial.println("IMU数据发布失败,WiFi未连接");
    }
}

void MQTT::publishDeviceInfo(device_state_t device_state)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        if (!mqttClient.publish(mqtt_topic_device_info.c_str(), device.device_state_to_json().c_str()))
        {
            Serial.println("设备信息发布失败");
        }
    }
    else
    {
        // Serial.println("设备信息发布失败,WiFi未连接");
    }
}