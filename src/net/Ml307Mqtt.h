/*
 * ml307 MQTT 客户端
 * 使用 AT 指令实现 MQTT 功能
 * 参考ESP32 MQTT客户端设计模式
 */

#ifndef ML307_MQTT_H
#define ML307_MQTT_H

#include <Arduino.h>
#include <functional>
#include "Ml307AtModem.h"

// MQTT事件类型定义
enum MqttEventType {
    MQTT_EVENT_CONNECTED = 1,
    MQTT_EVENT_DISCONNECTED = 2,
    MQTT_EVENT_ERROR = 3,
    MQTT_EVENT_MESSAGE = 4
};

// MQTT事件回调函数类型
using MqttEventCallback = std::function<void(MqttEventType event, const String& data)>;
using MqttMessageCallback = std::function<void(const String& topic, const String& payload)>;

class Ml307Mqtt {
public:
    Ml307Mqtt(Ml307AtModem& modem);
    
    // MQTT 连接操作 - 参考ESP32 MQTT设计
    bool connect(const char* broker, uint16_t port, 
                const char* clientId, 
                const char* username = nullptr, 
                const char* password = nullptr);
    void disconnect();
    
    // MQTT 消息操作
    bool publish(const char* topic, const char* payload, bool retain = false);
    bool subscribe(const char* topic, uint8_t qos = 0);
    bool unsubscribe(const char* topic);
    
    // 回调设置
    void onEvent(MqttEventCallback callback);
    void onMessage(MqttMessageCallback callback);
    
    // 状态检查
    bool connected();
    bool canPublish();
    bool canRead();
    
    // 消息处理
    void loop();

    // 设置调试模式
    void setDebug(bool debug);

private:
    Ml307AtModem& _modem;
    MqttEventCallback _eventCallback;
    MqttMessageCallback _messageCallback;
    
    // 连接状态管理
    bool _isConnected;
    bool _canPublish;
    unsigned long _lastCanPublishCheckTime;
    unsigned long _connectStartTime;
    bool _debug;
    
    // MQTT 配置
    String _clientId;
    String _username;
    String _password;
    String _broker;
    uint16_t _port;
    
    // 连接超时配置
    static const unsigned long MQTT_CONNECT_TIMEOUT_MS = 30000; // 30秒连接超时
    static const unsigned long MQTT_STATE_CHECK_INTERVAL = 500; // 状态检查间隔
    
    // 内部方法
    void handleEvent(MqttEventType event, const String& data = "");
    void handleMessage(const String& topic, const String& payload);
    bool parseMessageData(const String& data);
    bool configureConnection();
    void debugPrint(const String& msg);
    String encodeHex(const char* payload);
    void disconnectAndWait();
    
    // 连接流程方法
    bool waitForConnection();
    bool checkConnectionState();
    int getConnectionState(); // 新增：获取连接状态
    void resetConnectionState();
};

extern Ml307Mqtt ml307Mqtt;

#endif // ML307_MQTT_H 