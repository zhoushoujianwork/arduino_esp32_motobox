#include <WiFi.h>
#include <PubSubClient.h>
#include "MQTT.h"
#include "config.h"
#include "wifi/WifiManager.h"
#include <ArduinoJson.h>
#include "net/NetManager.h"

extern NetManager netManager;

MQTT::MQTT(const char *server, int port, const char *user, const char *password)
    : mqtt_server(server), mqtt_port(port), mqtt_user(user), mqtt_password(password)
{
    // 默认使用 WiFi
    if (port == 8883)
        mqttClient.setClient(wifiManager.wifiClientSecure);
    else
        mqttClient.setClient(wifiManager.wifiClient);
    networkClient = nullptr;
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

void MQTT::setClient(Client* client) {
    if (client && client != networkClient) {
        networkClient = client;
        mqttClient.setClient(*client);
    }
}

bool MQTT::reconnect()
{
    if (!netManager.isConnected())
    {
        Serial.println("[MQTT] 无可用网络连接，MQTT连接失败");
        return false;
    }

    NetManager::NetType currentNetType = netManager.getCurrentType();
    Serial.printf("[MQTT] 正在通过%s连接到服务器: %s:%d\n",
        currentNetType == NetManager::NET_WIFI ? "WiFi" : "4G",
        mqtt_server,
        mqtt_port);

    // 增加重试机制
    int attempts = 0;
    const int MAX_ATTEMPTS = 3;

    mqttClient.setSocketTimeout(15); // 设置15秒超时

    while (attempts < MAX_ATTEMPTS) {
        attempts++;
        Serial.printf("[MQTT] 第%d次尝试连接...\n", attempts);
        
        if (netManager.isConnected() && 
            mqttClient.connect(device.get_device_id().c_str(), mqtt_user, mqtt_password))
        {
            Serial.printf("[MQTT] 通过%s网络连接成功\n",
                currentNetType == NetManager::NET_WIFI ? "WiFi" : "4G");
            subscribeCommand();
            return true;
        }
        
        Serial.printf("[MQTT] 连接失败，错误码=%d\n", mqttClient.state());
        Serial.println("错误码说明:");
        Serial.println(" -4=连接超时");
        Serial.println(" -2=连接被拒绝");
        Serial.println(" -3=服务器不可达");
        Serial.println(" -1=MQTT版本不支持");
        
        if (attempts < MAX_ATTEMPTS) {
            Serial.println("[MQTT] 2秒后重试...");
            delay(2000);
        }
    }

    mqttClient.disconnect();
    return false;
}

void MQTT::loop()
{
    // 优先判断网络状态
    if (!netManager.isConnected()) {
        // Serial.println("[MQTT] 当前无可用网络连接");
        disconnect();
        return;
    }

    // 检查底层 Client 是否变化，并输出详细的网络状态
    Client* activeClient = netManager.getActiveClient();
    NetManager::NetType currentNetType = netManager.getCurrentType();
    
    // 输出当前网络状态
    // Serial.print("[MQTT] 当前网络类型: ");
    // switch(currentNetType) {
    //     case NetManager::NET_WIFI:
    //         Serial.printf("WiFi (SSID: %s, 信号强度: %d)\n", 
    //             WiFi.SSID().c_str(), 
    //             WiFi.RSSI());
    //         break;
    //     case NetManager::NET_GSM:
    //         Serial.println("4G/GSM");
    //         // 如果有信号强度检测函数，可以加入
    //         // Serial.printf("(信号强度: %d)\n", ml370.getSignalStrength());
    //         break;
    //     default:
    //         Serial.println("未知");
    // }

    // Client 发生变化时的详细日志
    if (activeClient != networkClient) {
        Serial.println("[MQTT] 网络切换检测到");
        Serial.printf("[MQTT] 从 %s 切换到 %s\n", 
            networkClient == &wifiManager.wifiClient ? "WiFi" : 
            networkClient == nullptr ? "无连接" : "4G",
            activeClient == &wifiManager.wifiClient ? "WiFi" : "4G");
        
        setClient(activeClient);
        disconnect();
    }

    // MQTT 连接状态检查和重连
    if (!mqttClient.connected())
    {
        Serial.printf("[MQTT] 连接断开(错误码:%d)，通过%s网络尝试重新连接\n", 
            mqttClient.state(),
            currentNetType == NetManager::NET_WIFI ? "WiFi" : "4G");
            
        if (!reconnect())
        {
            Serial.printf("[MQTT] %s网络连接失败\n", 
                currentNetType == NetManager::NET_WIFI ? "WiFi" : "4G");
            return;
        }
    }

    if (!mqttClient.loop())
    {
        Serial.printf("[MQTT] 循环失败 (当前网络: %s)\n",
            currentNetType == NetManager::NET_WIFI ? "WiFi" : "4G");
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
        powerManager.setSleepTime(sleepTime); // 通过powerManager统一设置和保存
        Serial.printf("设置休眠时间: %d 秒\n", sleepTime);
    } else {
        Serial.println("未知命令");
    }
}

void MQTT::disconnect() {
    if (mqttClient.connected()) {
        mqttClient.disconnect();
        Serial.println("低功耗/网络断开，已断开MQTT连接");
    }
}