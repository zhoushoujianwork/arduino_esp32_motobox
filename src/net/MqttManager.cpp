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
    for (int j = 0; j < 3; j++)
    {
        Serial.printf("尝试连接WiFi: %s\n", ssid.c_str());
        WiFi.begin(ssid.c_str(), password.c_str());
        for (int k = 0; k < 10; k++)
        {
            if (WiFi.status() == WL_CONNECTED)
            {
                Serial.printf("WiFi连接成功: %s, IP: %s\n", ssid.c_str(), WiFi.localIP().toString().c_str());
                return true;
            }
            delay(1000);
        }
        Serial.printf("WiFi连接失败: %s\n", ssid.c_str());
    }
    return false;
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
            if (_networkStateCallback)
            {
                _networkStateCallback(NetworkState::ERROR);
            }
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
        if (_networkStateCallback)
        {
            _networkStateCallback(NetworkState::ERROR);
        }
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
    if (_connectCallback)
    { // 添加空指针检查
        _connectCallback(success);
    }else{
        debugPrint("_connectCallback is null");
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

void MqttManager::loop()
{
    // 如果处于配网模式，直接返回，不做任何网络连接相关操作
#ifdef ENABLE_WIFI
    if (wifiManager.getConfigMode())
    {
        return;
    }
#endif

    if (!_isInitialized)
    {
        debugPrint("MqttManager loop not initialized");
        return;
    }

    // 网络连接处理是 2s 一次

    if (millis() - _lastNetworkCheckTime > NETWORK_CHECK_INTERVAL)
    {
        _lastNetworkCheckTime = millis();
        debugPrint("MqttManager loop network check");
        debugPrint("MqttManager loop network state: " + String((int)_networkState));
        // 处理网络连接状态
        switch (_networkState)
        {
        case NetworkState::CONNECTING:
        {
#ifdef ENABLE_WIFI
            if (WiFi.status() == WL_CONNECTED)
            {
                _networkState = NetworkState::CONNECTED;
                debugPrint("WiFi connected: " + WiFi.localIP().toString());
                if (_networkStateCallback)
                {
                    _networkStateCallback(_networkState);
                }
            }
            else if (millis() - _connectionStartTime > _connectionTimeout)
            {
                _networkState = NetworkState::ERROR;
                debugPrint("WiFi connection timeout");
                if (_networkStateCallback)
                {
                    _networkStateCallback(_networkState);
                }
            }
#else
            // 4G 连接状态检查
            if (ml307.isNetworkReady())
            {
                _networkState = NetworkState::CONNECTED;
                debugPrint("Cellular connected, CSQ: " + String(ml307.getCSQ()));
                if (_networkStateCallback)
                {
                    _networkStateCallback(_networkState);
                }
            }
            else if (millis() - _connectionStartTime > _connectionTimeout)
            {
                _networkState = NetworkState::ERROR;
                debugPrint("Cellular connection timeout");
                if (_networkStateCallback)
                {
                    _networkStateCallback(_networkState);
                }
            }
#endif
            break;
        }

        case NetworkState::CONNECTED:
        {
// 检查网络是否断开
// bool isConnected = (_config.networkType == MqttNetworkType::WIFI)
//     ? (WiFi.status() == WL_CONNECTED)
//     : (Ml307Mqtt.isConnected());
// if (!isConnected) {
//     _networkState = NetworkState::DISCONNECTED;
//     debugPrint("Network disconnected");
//     if (_networkStateCallback) {
//         _networkStateCallback(_networkState);
//     }
// }

// MQTT 相关处理
#ifdef ENABLE_WIFI
            if (_wifiMqttClient && !_wifiMqttClient->connected())
            {
                if (!reconnect())
                {
                    debugPrint("MqttManager loop reconnect failed");
                    // _networkState = NetworkState::ERROR;
                    if (_networkStateCallback)
                    {
                        _networkStateCallback(_networkState);
                    }
                }
            }
            if (_wifiMqttClient)
            {
                _wifiMqttClient->loop();
            }
#else
            debugPrint("MqttManager loop 4G");
            ml307Mqtt.loop();
#endif
            break;
        }

        case NetworkState::DISCONNECTED:
        {
            // 尝试重新连接
            unsigned long now = millis();
            if (now - _lastReconnectAttempt > RECONNECT_INTERVAL)
            {
                _lastReconnectAttempt = now;
                _networkState = NetworkState::CONNECTING;
                _connectionStartTime = now;

#ifdef ENABLE_WIFI
                // 这里还要加入一定时间间隔，避免频繁重连，不能直接 delay 阻塞其他
                if (now - _lastReconnectTime > RECONNECT_INTERVAL)
                {
                    _lastReconnectTime = now;
                }
                else
                {
                    break;
                }
                connectWifi();
#endif
                // 4G模式下，会自动尝试重连
            }
            break;
        }

        case NetworkState::ERROR:
        {
            // 错误状态下等待一段时间后重试
            if (millis() - _lastReconnectAttempt > RECONNECT_INTERVAL * 2)
            {
                _networkState = NetworkState::DISCONNECTED;
            }
            break;
        }
        }
    }

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
            // 你可以继续添加更多主题类型
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

// 检查MQTT连接状态
#ifdef ENABLE_WIFI
    if (!_wifiMqttClient)
    {
        debugPrint("MqttManager 客户端未初始化");
        return false;
    }
    if (!_wifiMqttClient->connected())
    {
        debugPrint("MqttManager 客户端未连接");
        return false;
    }
#else
    if (!ml307Mqtt.connected())
    {
        debugPrint("MqttManager 4G MQTT未连接");
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
