/*
 * ml307 MQTT 客户端
 * 使用 AT 指令实现 MQTT 功能
 */

#ifndef ML307_MQTT_H
#define ML307_MQTT_H

#include <Arduino.h>
#include <functional>
#include "Ml307AtModem.h"

using MqttMessageCallback = std::function<void(const String& topic, const String& payload)>;

class Ml307Mqtt {
public:
    Ml307Mqtt(Ml307AtModem& modem);
    
    // MQTT 操作
    bool connect(const char* broker, uint16_t port, 
                const char* clientId, 
                const char* username = nullptr, 
                const char* password = nullptr);
    bool publish(const char* topic, const char* payload, bool retain = false);
    bool subscribe(const char* topic, uint8_t qos = 0);
    bool unsubscribe(const char* topic);
    
    // 回调设置
    void onMessage(MqttMessageCallback callback);
    
    // 状态检查
    bool connected();
    bool isConnected();
    
    // 消息处理
    void loop();

    // 设置调试模式
    void setDebug(bool debug);

private:
    Ml307AtModem& _modem;
    MqttMessageCallback _messageCallback;
    bool _connected;
    unsigned long _lastCheckTime;
    bool _debug;
    
    // MQTT 配置
    String _clientId;
    String _username;
    String _password;
    
    // 内部方法
    void handleMessage(const String& topic, const String& payload);
    bool parseMessageData(const String& data);
    bool configureConnection();
    void debugPrint(const String& msg);
    String encodeHex(const char* payload);
    void disconnectAndWait();
};

extern Ml307Mqtt ml307Mqtt;

#endif // ML307_MQTT_H 