/*
 * MQTT 管理器
 * 支持 WiFi 和 4G 两种网络方式
 */

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <map>
#include "Ml307Mqtt.h"
#include "config.h"
#include "device.h"
#include "imu/qmi8658.h"
#include "gps/GPS.h"
#include "utils/PreferencesUtils.h"

// MQTT 网络类型
enum class MqttNetworkType {
    WIFI,
    CELLULAR,
    NONE
};

// 添加连接状态枚举
enum class NetworkState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR
};

// MQTT 配置
struct MqttManagerConfig {
    
    // MQTT 配置
    const char* broker;
    uint16_t port;
    const char* clientId;
    const char* username;
    const char* password;
    uint16_t keepAlive;
    bool cleanSession;
    
    MqttManagerConfig() 
        : broker(nullptr)
        , port(1883)
        , clientId(nullptr)
        , username(nullptr)
        , password(nullptr)
        , keepAlive(60)
        , cleanSession(true) {}
};

// 添加主题发布配置结构
struct TopicConfig {
    String topic;
    unsigned long interval;
    unsigned long lastPublishTime;
};

class MqttManager {
public:
    // 构造函数
    MqttManager();
    ~MqttManager();
    
    // 初始化
    bool begin(const MqttManagerConfig& config);
    
    // 连接操作
    bool connect();
    // void disconnect();
    // bool isConnected();
    
    // MQTT 操作
    bool publish(const char* topic, const char* payload, bool retain = false);
    bool subscribe(const char* topic, uint8_t qos = 0);
    bool unsubscribe(const char* topic);
    
    // 回调设置
    void onMessage(MqttMessageCallback callback);
    void onConnect(std::function<void(bool)> callback);
    
    // 消息处理
    void loop();

    // 网络状态
    bool isNetworkConnected() const;
    String getNetworkInfo() const;
    
    // 添加网络状态回调设置
    void onNetworkState(std::function<void(NetworkState)> callback);
    
    // 获取当前网络状态
    NetworkState getNetworkState() const { return _networkState; }
    
    // 添加主题配置
    void addTopic(const String& name, const String& topic, unsigned long interval);
    
    // 发布指定主题的数据
    bool publishToTopic(const String& name, const char* payload, bool retain = false);
    
    // 调试功能
    void setDebug(bool enable) { _debug = enable; }

    // Add new method after other public methods
    void onWiFiEvent(WiFiEvent_t event);

private:
    MqttManagerConfig _config;
    WiFiClient _wifiClient;
    PubSubClient* _wifiMqttClient;
    bool _isInitialized;

    unsigned long _lastNetworkCheckTime;
    static const unsigned long NETWORK_CHECK_INTERVAL = 2000; // 2秒检查一次
    
    // 回调函数
    static void wifiMqttCallback(char* topic, byte* payload, unsigned int length);
    static MqttMessageCallback _messageCallback;
    std::function<void(bool)> _connectCallback;
    
    // 内部方法
    bool initWifi();
    bool initCellular();
    void cleanup();
    // WiFi 相关
    bool connectWifi();
    void disconnectWifi();
    
    // 状态检查
    // bool checkWifiStatus();
    // bool checkCellularStatus();
    
    // 重连逻辑
    bool reconnect();
    unsigned long _lastReconnectAttempt;
    unsigned long _lastReconnectTime;
    static const unsigned long RECONNECT_INTERVAL = 5000; // 5秒重连间隔
    
    // 调试输出
    void debugPrint(const String& msg);
    bool _debug;
    
    NetworkState _networkState;
    unsigned long _connectionStartTime;
    uint32_t _connectionTimeout;
    
    // 网络状态回调
    std::function<void(NetworkState)> _networkStateCallback;
    
    // 主题配置映射
    std::map<String, TopicConfig> _topicConfigs;

    // 添加连接状态跟踪
    bool _lastConnectionState;

    void setNetworkState(NetworkState newState);
};

// 全局实例
extern MqttManager mqttManager;

#endif // MQTT_MANAGER_H
