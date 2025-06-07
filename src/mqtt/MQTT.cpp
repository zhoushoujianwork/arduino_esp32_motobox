#include <WiFi.h>
#include <PubSubClient.h>
#include "MQTT.h"
#include "config.h"
#include "wifi/WifiManager.h"
#include <ArduinoJson.h>

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
    mqtt_topic_command = MQTT_TOPIC_COMMAND;

    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->handleCommand(topic, payload, length);
    });
}

bool MQTT::reconnect()
{
    if (!device.get_device_state()->wifiConnected)
    {
        Serial.println("WiFi未连接，MQTT连接失败");
        return false;
    }

    Serial.println("正在连接MQTT服务器...");
    Serial.print("尝试SSL连接到服务器: ");
    Serial.print(mqtt_server);
    Serial.print(":");
    Serial.println(mqtt_port);

    // 增加重试机制
    int attempts = 0;
    const int MAX_ATTEMPTS = 3;

    mqttClient.setSocketTimeout(15); // 设置15秒超时

    while (attempts < MAX_ATTEMPTS) {
        if (device.get_device_state()->wifiConnected && mqttClient.connect(device.get_device_id().c_str(), mqtt_user, mqtt_password))
        {
            Serial.println("重试 MQTT连接成功");
            subscribeCommand();

            return true;
        }
        
        Serial.print("连接失败，错误码=");
        Serial.print(mqttClient.state());
        Serial.println(" (提示:-4=连接超时,-2=连接被拒绝,-3=服务器不可达)");
        
        attempts++;
        delay(2000); // 每次重试间隔2秒
    }

    mqttClient.disconnect();
    return false;
}

void MQTT::loop()
{
    // 如果WiFi未连接，直接返回，不做任何MQTT操作
    if (!device.get_device_state()->wifiConnected)
    {
        // 只在第一次WiFi断开时断开MQTT，避免反复调用
        if (mqttClient.connected()) {
            mqttClient.disconnect();
            Serial.println("WiFi断开，已断开MQTT连接");
        }
        return;
    }

    // 只有WiFi连接时才处理MQTT连接和循环
    if (!mqttClient.connected())
    {
        Serial.println("MQTT连接断开，尝试重新连接");
        if (!reconnect())
        {
            Serial.println("MQTT连接失败");
            return;
        }
    }

    if (!mqttClient.loop())
    {
        Serial.println("MQTT循环失败");
    }
}

void MQTT::publishGPS(gps_data_t gps_data)
{
    if (device.get_device_state()->wifiConnected)
    {
       
        if (!mqttClient.publish(mqtt_topic_gps.c_str(), device.gps_data_to_json().c_str()))
        {
            Serial.println("GPS数据发布失败");
            device.printGpsData();
            Serial.println("publishGPS");
            Serial.println(mqtt_topic_gps.c_str());
            Serial.println(device.gps_data_to_json().c_str());
            Serial.println(device.gps_data_to_json().length());
        }
    }
    else
    {
        // Serial.println("GPS数据发布失败,WiFi未连接");
    }
}

void MQTT::publishIMU(imu_data_t imu_data)
{
    if (device.get_device_state()->wifiConnected)
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
    if (device.get_device_state()->wifiConnected)
    {
        if (!mqttClient.publish(mqtt_topic_device_info.c_str(), device.device_state_to_json().c_str(), true))
        {
            Serial.println(mqtt_topic_device_info.c_str());
            Serial.println(device.device_state_to_json().c_str());
            Serial.println(device.device_state_to_json().length());
            Serial.println("设备信息发布失败");
        }
        else
        {
            // Serial.println(mqttClient.connected() ? "MQTT已连接" : "MQTT未连接");
        }
    }
    else
    {
        // Serial.println("设备信息发布失败,WiFi未连接");
    }
}

/**
 * @brief 订阅MQTT命令下发主题
 */
void MQTT::subscribeCommand() {
    // 构建命令topic，例如 device/{device_id}/cmd
    mqttClient.subscribe(mqtt_topic_command.c_str());
    Serial.print("已订阅命令主题: ");
    Serial.println(mqtt_topic_command);
}

/**
 * @brief 处理MQTT下发的命令（JSON格式）
 * @param topic 主题
 * @param payload 消息内容
 * @param length 消息长度
 */
void MQTT::handleCommand(char* topic, byte* payload, unsigned int length) {
    // 将payload转为字符串
    String msg;
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    Serial.print("收到命令消息: ");
    Serial.println(msg);

    // 使用 ArduinoJson 解析JSON命令
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
        Serial.print("JSON解析失败: ");
        Serial.println(error.c_str());
        return;
    }

    // 获取cmd字段
    const char* cmd = doc["cmd"];
    if (!cmd) {
        Serial.println("未找到cmd字段");
        return;
    }

    // 匹配命令
    if (strcmp(cmd, "reset_wifi") == 0) {
        Serial.println("执行: 重置WiFi");
        wifiManager.reset();
    } else if (strcmp(cmd, "enter_config") == 0) {
        Serial.println("执行: 进入配网模式");
        wifiManager.enterConfigMode();
    } else if (strcmp(cmd, "exit_config") == 0) {
        Serial.println("执行: 退出配网模式");
        wifiManager.exitConfigMode();
    } else if (strcmp(cmd, "enter_sleep") == 0) {
        Serial.println("执行: 进入睡眠模式");
        // powerManager.requestLowPowerMode = true;
        // powerManager.enterLowPowerMode();
    } else if (strcmp(cmd, "set_sleep_time") == 0) {
        // 获取sleep_time字段
        int sleepTime = doc["sleep_time"] | 0; // 默认0
        device.get_device_state()->sleep_time = sleepTime;
        Serial.printf("设置睡眠时间: %d 秒\n", sleepTime);
    } else {
        Serial.println("未知命令");
    }
}