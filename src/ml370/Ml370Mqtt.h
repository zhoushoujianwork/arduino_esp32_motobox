/*
 * ML370 MQTT 客户端
 * 使用 AT 指令实现 MQTT 功能
 */

#ifndef ML370_MQTT_H
#define ML370_MQTT_H

#include <Arduino.h>
#include <functional>
#include "Ml370AtModem.h"

using MqttMessageCallback = std::function<void(const String& topic, const String& payload)>;

class Ml370Mqtt {
public:
    Ml370Mqtt(Ml370AtModem& modem);
    
    // MQTT 操作
    bool connect(const char* broker, uint16_t port, 
                const char* clientId, 
                const char* username = nullptr, 
                const char* password = nullptr);
    void disconnect();
    bool publish(const char* topic, const char* payload, bool retain = false);
    bool subscribe(const char* topic, uint8_t qos = 0);
    bool unsubscribe(const char* topic);
    
    // 回调设置
    void onMessage(MqttMessageCallback callback);
    
    // 状态检查
    bool connected();
    
    // 消息处理
    void loop();

private:
    Ml370AtModem& _modem;
    MqttMessageCallback _messageCallback;
    bool _connected;
    
    // MQTT 配置
    String _clientId;
    String _username;
    String _password;
    
    // 内部方法
    void handleMessage(const String& topic, const String& payload);
    bool parseMessageData(const String& data);
    bool configureConnection();
};

extern Ml370Mqtt ml370Mqtt;

#endif // ML370_MQTT_H 