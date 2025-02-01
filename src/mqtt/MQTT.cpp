#include <WiFi.h>
#include <AsyncMqttClient.h>

AsyncMqttClient mqttClient;

void connectToMqtt()
{
    mqttClient.setServer("192.168.1.100", 1883);
    mqttClient.setCredentials("device_id", "secure_password");
    mqttClient.connect();
}

void onMqttConnect(bool sessionPresent)
{
    mqttClient.subscribe("vehicle/ESP32-S3/status", 1);
    mqttClient.publish("vehicle/ESP32-S3/data", 1, true, "{\"speed\":60}");
}