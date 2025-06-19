/*
 * MQTT 管理器实现
 */

#include "MqttManager.h"

MqttManager mqttManager;
MqttMessageCallback MqttManager::_messageCallback = nullptr;

MqttManager::MqttManager()
    : _wifiMqttClient(nullptr), _isInitialized(false), _lastReconnectAttempt(0),
      _debug(false),
      _connectCallback(nullptr), _networkState(NetworkState::DISCONNECTED),
      _connectionStartTime(0), _connectionTimeout(30000)
{
}

MqttManager::~MqttManager()
{
    cleanup();
}

void MqttManager::cleanup()
{
    if (_wifiMqttClient)
    {
        delete _wifiMqttClient;
        _wifiMqttClient = nullptr;
    }
    _isInitialized = false;
}

bool MqttManager::begin(const MqttManagerConfig &config)
{
    // setDebug(true);
    Serial.println("[MqttManager] begin");
    cleanup();
    _config = config;
    // 根据配置选择网络方式
    bool result = false;
#ifdef ENABLE_WIFI
    result = initWifi();
#else
    result = initCellular();
#endif

    Serial.println("[MqttManager] begin result: " + String(result));

    _isInitialized = result;

    if (result)
    {
        connect();
    }

    return result;
}

bool MqttManager::initWifi()
{
    Serial.println("[MqttManager] initWifi");

    // 创建 MQTT 客户端
    _wifiMqttClient = new PubSubClient(_wifiClient);
    _wifiMqttClient->setServer(_config.broker, _config.port);
    _wifiMqttClient->setCallback(wifiMqttCallback);
    _wifiMqttClient->setKeepAlive(_config.keepAlive);

    _connectionStartTime = millis();
    return true;
}

bool MqttManager::initCellular()
{
    debugPrint("MqttManager initCellular");
    _networkState = NetworkState::CONNECTING;
    _connectionStartTime = millis();

    return true;
}

bool MqttManager::connectWifi()
{
    String ssid, password;
    if (!PreferencesUtils::getWifi(ssid, password))
    {
        Serial.println("[MqttManager] 未找到已保存的WiFi配置");
#ifdef ENABLE_WIFI
        wifiManager.enterConfigMode();
#endif
        return false;
    }
    
    Serial.printf("[MqttManager] 读取到WiFi配置: %s\n", ssid.c_str());
    
    // 非阻塞连接，只尝试一次
    Serial.printf("尝试连接WiFi: %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // 设置连接超时时间
    _connectionStartTime = millis();
    _networkState = NetworkState::CONNECTING;
    
    if (_networkStateCallback)
    {
        _networkStateCallback(_networkState);
    }
    
    // 立即返回，让 loop() 函数处理连接状态检查
    return true;
}

void MqttManager::disconnectWifi()
{
    WiFi.disconnect();
}

bool MqttManager::connect()
{
    if (!_isInitialized)
    {
        debugPrint("Not initialized");
        return false;
    }

    bool success = false;
#ifdef ENABLE_WIFI
    debugPrint("MqttManager connect WiFi");
    if (!_wifiMqttClient)
        return false;

    // 添加错误处理
    if (WiFi.status() != WL_CONNECTED)
    {
        debugPrint("WiFi not connected, attempting to connect...");
        if (!connectWifi())
        {
            debugPrint("WiFi connection failed");
            setNetworkState(NetworkState::ERROR);
            return false;
        }
    }

    success = _wifiMqttClient->connect(
        _config.clientId,
        _config.username,
        _config.password,
        nullptr, // will topic
        0,       // will qos
        false,   // will retain
        nullptr, // will message
        _config.cleanSession);

    if (!success)
    {
        debugPrint("MQTT connection failed");
        debugPrint("MQTT broker: " + String(_config.broker));
        debugPrint("MQTT port: " + String(_config.port));
        debugPrint("MQTT clientId: " + String(_config.clientId));
        debugPrint("MQTT username: " + String(_config.username));
        setNetworkState(NetworkState::ERROR);
    }
#else
    debugPrint("MqttManager connect 4G");
    debugPrint("MqttManager connect 4G broker: " + String(_config.broker));
    debugPrint("MqttManager connect 4G port: " + String(_config.port));
    debugPrint("MqttManager connect 4G clientId: " + String(_config.clientId));
    debugPrint("MqttManager connect 4G username: " + String(_config.username));
    debugPrint("MqttManager connect 4G password: " + String(_config.password));
    success = ml307Mqtt.connect(
        _config.broker,
        _config.port,
        _config.clientId,
        _config.username,
        _config.password);
#endif

    debugPrint("MqttManager connect callback success");
    if (success)
    {
        setNetworkState(NetworkState::CONNECTED);
        // 添加MQTT连接回调
        if (_connectCallback)
        {
            _connectCallback(true);
        }
    }
    return success;
}

bool MqttManager::reconnect()
{
    unsigned long now = millis();
    if (now - _lastReconnectAttempt < RECONNECT_INTERVAL)
    {
        return false;
    }
    _lastReconnectAttempt = now;

    return connect();
}

// bool MqttManager::isConnected() {
//     return _networkState == NetworkState::CONNECTED;
// }

void MqttManager::setNetworkState(NetworkState newState) {
    if (_networkState != newState) {
        NetworkState oldState = _networkState;
        _networkState = newState;
        debugPrint("网络状态变化: " + String((int)oldState) + " -> " + String((int)newState));
        if (_networkStateCallback) {
            _networkStateCallback(_networkState);
        }
    }
}

void MqttManager::loop()
{
#ifdef ENABLE_WIFI
    if (wifiManager.getConfigMode()) return;
#endif
    if (!_isInitialized) return;

    // 更快的检查间隔 - 改为200ms提高响应速度
    if (millis() - _lastNetworkCheckTime > 200) {
        _lastNetworkCheckTime = millis();

#ifdef ENABLE_WIFI
        // WiFi逻辑
        if (WiFi.status() != WL_CONNECTED) {
            setNetworkState(NetworkState::DISCONNECTED);
            if (_connectCallback) {
                _connectCallback(false);
            }
        } else {
            // WiFi已连接，检查MQTT状态
            if (_wifiMqttClient && !_wifiMqttClient->connected()) {
                setNetworkState(NetworkState::CONNECTING);
                reconnect();
            } else if (_wifiMqttClient && _wifiMqttClient->connected()) {
                setNetworkState(NetworkState::CONNECTED);
            }
        }
        if (_wifiMqttClient) _wifiMqttClient->loop();
#else
        // 4G逻辑
        switch (_networkState)
        {
        case NetworkState::CONNECTING:
        {
            if (ml307.isNetworkReady())
            {
                setNetworkState(NetworkState::CONNECTED);
                debugPrint("Cellular connected, CSQ: " + String(ml307.getCSQ()));
            }
            else if (millis() - _connectionStartTime > _connectionTimeout)
            {
                setNetworkState(NetworkState::ERROR);
                debugPrint("Cellular connection timeout");
            }
            break;
        }

        case NetworkState::CONNECTED:
        {
            if (!ml307.isNetworkReady())
            {
                setNetworkState(NetworkState::DISCONNECTED);
                debugPrint("Cellular disconnected");
            }
            ml307Mqtt.loop();
            break;
        }

        case NetworkState::DISCONNECTED:
        {
            if (_connectCallback) {
                _connectCallback(false);
            }
            // 尝试重新连接
            unsigned long now = millis();
            if (now - _lastReconnectAttempt > RECONNECT_INTERVAL)
            {
                _lastReconnectAttempt = now;
                setNetworkState(NetworkState::CONNECTING);
                _connectionStartTime = now;
            }
            break;
        }

        case NetworkState::ERROR:
        {
            // 错误状态下等待一段时间后重试
            if (millis() - _lastReconnectAttempt > RECONNECT_INTERVAL * 2)
            {
                setNetworkState(NetworkState::DISCONNECTED);
            }
            break;
        }
        }
#endif
    }

    if (_networkState != NetworkState::CONNECTED) return;

    // 处理预注册的主题
    for (auto &topic : _topicConfigs)
    {
        if (topic.second.lastPublishTime + topic.second.interval < millis())
        {
            if (topic.first == "device_info")
            {
                publishToTopic(topic.first, device_state_to_json(get_device_state()).c_str(), false);
            }
            else if (topic.first == "imu")
            {
                publishToTopic(topic.first, imu_data_to_json(imu_data).c_str(), false);
            }
            else if (topic.first == "gps")
            {
                publishToTopic(topic.first, gps_data_to_json(gps_data).c_str(), false);
            }
        }
    }
}

bool MqttManager::publish(const char *topic, const char *payload, bool retain)
{
    debugPrint("MqttManager publish topic: " + String(topic) + ", payload: " + String(payload) + ", retain: " + String(retain));
#ifdef ENABLE_WIFI
    if (!_wifiMqttClient)
    {
        debugPrint("MQTT client is null");
        return false;
    }
    bool result = _wifiMqttClient->publish(topic, payload, retain);
    if (!result)
    {
        debugPrint("WiFi MQTT publish failed - Topic: " + String(topic));
    }
    return result;
#else
    bool result = ml307Mqtt.publish(topic, payload, retain);
    if (!result)
    {
        debugPrint("Cellular MQTT publish failed - Topic: " + String(topic));
    }
    return result;
#endif
}

bool MqttManager::subscribe(const char *topic, uint8_t qos)
{
    Serial.printf("[MqttManager] 订阅主题: %s, 质量: %d\n", topic, qos);
#ifdef ENABLE_WIFI
    return _wifiMqttClient && _wifiMqttClient->subscribe(topic, qos);
#else
    return ml307Mqtt.subscribe(topic, qos);
#endif
}

bool MqttManager::unsubscribe(const char *topic)
{
    Serial.printf("[MqttManager] 取消订阅主题: %s\n", topic);
#ifdef ENABLE_WIFI
    return _wifiMqttClient && _wifiMqttClient->unsubscribe(topic);
#else
    return ml307Mqtt.unsubscribe(topic);
#endif
}

void MqttManager::onMessage(MqttMessageCallback callback)
{
    _messageCallback = callback;
#ifdef ENABLE_GSM
    ml307Mqtt.onMessage(callback);
#endif
}

void MqttManager::onConnect(std::function<void(bool)> callback)
{
    _connectCallback = callback;
}

bool MqttManager::isNetworkConnected() const
{
#ifdef ENABLE_WIFI
    return WiFi.status() == WL_CONNECTED;
#else
    return ml307Mqtt.isConnected();
#endif
    return false;
}

String MqttManager::getNetworkInfo() const
{
    String info;
#ifdef ENABLE_WIFI
    info = "WiFi SSID: " + String(WiFi.SSID());
    info += ", IP: " + WiFi.localIP().toString();
    info += ", RSSI: " + String(WiFi.RSSI());
#else
    info = "Cellular CSQ: " + String(ml307.getCSQ());
    info += ", IP: " + ml307.getLocalIP();
#endif
    return info;
}

void MqttManager::debugPrint(const String &msg)
{
    if (_debug)
    {
        Serial.println("[MqttManager] [debug]:  " + msg);
    }
}

void MqttManager::wifiMqttCallback(char *topic, byte *payload, unsigned int length)
{
    if (_messageCallback)
    {
        String payloadStr;
        payloadStr.reserve(length);
        for (unsigned int i = 0; i < length; i++)
        {
            payloadStr += (char)payload[i];
        }
        _messageCallback(topic, payloadStr);
    }
}

void MqttManager::onNetworkState(std::function<void(NetworkState)> callback)
{
    _networkStateCallback = callback;
}

void MqttManager::addTopic(const String &name, const String &topic, unsigned long interval)
{
    TopicConfig config = {
        .topic = topic,
        .interval = interval,
        .lastPublishTime = 0};
    _topicConfigs[name] = config;
    Serial.printf("[MqttManager] 添加主题: %s, 主题: %s, 间隔: %d\n", name.c_str(), topic.c_str(), interval);
}

bool MqttManager::publishToTopic(const String &name, const char *payload, bool retain)
{
    auto it = _topicConfigs.find(name);
    if (it == _topicConfigs.end())
    {
        debugPrint("MqttManager 主题未找到: " + name);
        return false;
    }

    // 检查MQTT连接状态 - 使用setNetworkState而不是直接赋值
#ifdef ENABLE_WIFI
    if (!_wifiMqttClient)
    {
        debugPrint("MqttManager 客户端未初始化");
        return false;
    }
    if (!_wifiMqttClient->connected())
    {
        debugPrint("MqttManager 客户端未连接");
        setNetworkState(NetworkState::DISCONNECTED); // 使用setNetworkState
        return false;
    }
#else
    if (!ml307Mqtt.connected())
    {
        debugPrint("MqttManager 4G MQTT未连接");
        setNetworkState(NetworkState::DISCONNECTED); // 使用setNetworkState
        return false;
    }
#endif

    unsigned long now = millis();
    if (now - it->second.lastPublishTime < it->second.interval)
    {
        return true; // 未到发布时间，跳过
    }

    bool result = publish(it->second.topic.c_str(), payload, retain);
    if (result)
    {
        it->second.lastPublishTime = now;
        debugPrint("MqttManager 发布成功 - 主题: " + it->second.topic);
    }
    else
    {
        Serial.printf("[MqttManager] 发布失败 - 主题: %s, 网络类型: %s\n",
                      it->second.topic.c_str(),
#ifdef ENABLE_WIFI
                      "WiFi"
#else
                      "4G"
#endif
        );
    }
    return result;
}

// 添加新的方法：从WiFi事件处理器直接调用
void MqttManager::onWiFiEvent(WiFiEvent_t event)
{
#ifdef ENABLE_WIFI
    debugPrint("WiFi事件: " + String(event));
    switch (event)
    {
    case SYSTEM_EVENT_STA_CONNECTED:
        debugPrint("WiFi事件: 已连接到WiFi网络");
        setNetworkState(NetworkState::CONNECTING); // 先设为CONNECTING，等获取IP后再设为CONNECTED
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        debugPrint("WiFi事件: 获取到IP地址: " + WiFi.localIP().toString());
        setNetworkState(NetworkState::CONNECTED);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        debugPrint("WiFi事件: WiFi连接已断开");
        setNetworkState(NetworkState::DISCONNECTED);
        break;
    default:
        break;
    }
#endif
}
