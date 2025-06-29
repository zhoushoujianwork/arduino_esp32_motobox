/*
 * MQTT 管理器实现
 */

#include "MqttManager.h"

MqttManager mqttManager;
MqttMessageCallback MqttManager::_messageCallback = nullptr;

MqttManager::MqttManager()
    : _wifiMqttClient(nullptr), _isInitialized(false), _lastReconnectAttempt(0),
      _debug(false),
      _connectCallback(nullptr), _MqttState(MqttState::DISCONNECTED),
      _connectionStartTime(0), _connectionTimeout(30000),
      _lastConnectionState(false)
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
    _MqttState = MqttState::CONNECTING;
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
        wifiManager.enterAPMode();
#endif
        return false;
    }

    Serial.printf("[MqttManager] 读取到WiFi配置: %s\n", ssid.c_str());

    // 非阻塞连接，只尝试一次
    Serial.printf("尝试连接WiFi: %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());

    // 设置连接超时时间
    _connectionStartTime = millis();
    _MqttState = MqttState::CONNECTING;

    if (_MqttStateCallback)
    {
        _MqttStateCallback(_MqttState);
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
            setMqttState(MqttState::ERROR);
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
        setMqttState(MqttState::ERROR);
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
        setMqttState(MqttState::CONNECTED);
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
//     return _MqttState == MqttState::CONNECTED;
// }

void MqttManager::setMqttState(MqttState newState)
{
    if (_MqttState != newState)
    {
        MqttState oldState = _MqttState;
        _MqttState = newState;
        debugPrint("网络状态变化: " + String((int)oldState) + " -> " + String((int)newState));
        if (_MqttStateCallback)
        {
            _MqttStateCallback(_MqttState);
        }
    }
}

void MqttManager::loop()
{

    if (!_isInitialized)
        return;

    // 增加检查间隔，减少重复查询 - 从200ms改为2秒
    if (millis() - _lastNetworkCheckTime > 2000)
    {
        _lastNetworkCheckTime = millis();

#ifdef ENABLE_GSM
        if (!ml307_at.isNetworkReadyCheck())
        {
            Serial.println("[MqttManager] 网络未就绪, 设置为DISCONNECTED");
            setMqttState(MqttState::DISCONNECTED);
            return;
        }

        // 如果在获取lbs数据，则不进行mqtt的读取
        if (ml307_at.isLBSLoading())
            return;

        // 4G逻辑优化 - 减少重复查询
        bool currentConnected = (_MqttState == MqttState::CONNECTED) && isMqttConnected();

        if (currentConnected != _lastConnectionState)
        {
            _lastConnectionState = currentConnected;
            if (_connectCallback)
            {
                _connectCallback(currentConnected);
            }
        }

        debugPrint("MqttManager loop MqttState: " + String((int)_MqttState));
        switch (_MqttState)
        {
        case MqttState::CONNECTING:
        {
            // 只在连接超时时检查网络状态，避免频繁查询
            if (millis() - _connectionStartTime > _connectionTimeout)
            {
                setMqttState(MqttState::ERROR);
                debugPrint("Cellular connection timeout");
            }
            else
            {
                // 减少网络状态检查频率
                static unsigned long lastNetworkCheck = 0;
                if (millis() - lastNetworkCheck > 5000)
                { // 5秒检查一次
                    lastNetworkCheck = millis();

                    if (ml307_at.isNetworkReady())
                    {
                        if (ml307Mqtt.connected())
                        {
                            setMqttState(MqttState::CONNECTED);
                            debugPrint("Cellular connected, CSQ: " + String(ml307_at.getCSQ()));

                            if (_connectCallback)
                            {
                                _connectCallback(true);
                            }
                        }
                        else
                        {
                            debugPrint("网络已就绪，尝试MQTT连接...");
                            if (connect())
                            {
                                setMqttState(MqttState::CONNECTED);
                                debugPrint("MQTT连接成功");
                            }
                        }
                    }
                }
            }
            break;
        }

        case MqttState::CONNECTED:
        {
            // 连接正常，处理MQTT消息
            ml307Mqtt.loop();
            // 处理预注册的主题
            for (auto &topic : _topicConfigs)
            {
                if (topic.second.lastPublishTime + topic.second.interval < millis())
                {
                    if (topic.first == "device_info")
                    {
                        publishToTopic(topic.first, device_state_to_json(get_device_state()).c_str(), false);
                    }
                    // else if (topic.first == "imu")
                    // {
                    //     publishToTopic(topic.first, imu_data_to_json(imu_data).c_str(), false);
                    // }
                    else if (topic.first == "gps")
                    {
                        if (gps_data.satellites > 3 || lbs_data.valid)
                        {
                            publishToTopic(topic.first, gps_data_to_json(gps_data).c_str(), false);
                        }
                    }
                }
            }
            break;
        }

        case MqttState::DISCONNECTED:
        {
            // 检查网络是否恢复
            if (ml307_at.isNetworkReady())
            {
                debugPrint("网络已恢复，开始重新连接...");
                setMqttState(MqttState::CONNECTING);
                _connectionStartTime = millis();
            }
            else
            {
                // 尝试重新连接
                unsigned long now = millis();
                if (now - _lastReconnectAttempt > RECONNECT_INTERVAL)
                {
                    _lastReconnectAttempt = now;
                    debugPrint("尝试重新连接网络...");
                    // 这里可以添加网络重连逻辑
                }
            }
            break;
        }

        case MqttState::ERROR:
        {
            // 错误状态下等待一段时间后重试
            if (millis() - _lastReconnectAttempt > RECONNECT_INTERVAL * 2)
            {
                setMqttState(MqttState::DISCONNECTED);
                debugPrint("从错误状态恢复，重新开始连接");
            }
            break;
        }
        }

#else
        // WiFi逻辑

        if (wifiManager.getConfigMode())
            return;

        bool currentConnected = (WiFi.status() == WL_CONNECTED) &&
                                (_wifiMqttClient && _wifiMqttClient->connected());

        if (currentConnected != _lastConnectionState)
        {
            _lastConnectionState = currentConnected;
            if (_connectCallback)
            {
                _connectCallback(currentConnected);
            }
        }

        if (WiFi.status() != WL_CONNECTED)
        {
            setMqttState(MqttState::DISCONNECTED);
        }
        else
        {
            if (_wifiMqttClient && !_wifiMqttClient->connected())
            {
                setMqttState(MqttState::CONNECTING);
                reconnect();
            }
            else if (_wifiMqttClient && _wifiMqttClient->connected())
            {
                setMqttState(MqttState::CONNECTED);
            }
        }
        if (_wifiMqttClient)
            _wifiMqttClient->loop();
#endif
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
    return ml307Mqtt.canPublish();
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
    info = "Cellular CSQ: " + String(ml307_at.getCSQ());
    info += ", IP: " + ml307_at.getLocalIP();
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

void MqttManager::onMqttState(std::function<void(MqttState)> callback)
{
    _MqttStateCallback = callback;
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

    // 检查网络和MQTT连接状态
#ifdef ENABLE_WIFI
    if (!_wifiMqttClient || !_wifiMqttClient->connected())
    {
        debugPrint("MqttManager WiFi MQTT未连接");
        setMqttState(MqttState::DISCONNECTED);
        return false;
    }
#else
    // 4G模式下需要更严格的连接检查
    if (!ml307_at.isNetworkReady() || !ml307Mqtt.connected())
    {
        debugPrint("MqttManager 4G网络或MQTT未连接");
        setMqttState(MqttState::DISCONNECTED);
        return false;
    }

    // 添加MQTT连接状态验证
    if (!ml307Mqtt.canPublish())
    {
        debugPrint("MqttManager 4G MQTT连接验证失败");
        setMqttState(MqttState::DISCONNECTED);
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
        // 发布失败时，检查是否是MQTT连接问题
        debugPrint("MqttManager 发布失败，检查连接状态");
#ifdef ENABLE_WIFI
        if (!_wifiMqttClient->connected())
        {
            setMqttState(MqttState::DISCONNECTED);
        }
#else
        // 4G模式下，发布失败通常意味着MQTT连接断开
        setMqttState(MqttState::DISCONNECTED);
#endif
    }
    return result;
}

// 添加MQTT连接状态检查方法
bool MqttManager::isMqttConnected() const
{
#ifdef ENABLE_WIFI
    return _wifiMqttClient && _wifiMqttClient->connected();
#else
    return ml307Mqtt.connected() && ml307Mqtt.canPublish();
#endif
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
        setMqttState(MqttState::CONNECTING); // 先设为CONNECTING，等获取IP后再设为CONNECTED
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        debugPrint("WiFi事件: 获取到IP地址: " + WiFi.localIP().toString());
        setMqttState(MqttState::CONNECTED);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        debugPrint("WiFi事件: WiFi连接已断开");
        setMqttState(MqttState::DISCONNECTED);
        break;
    default:
        break;
    }
#endif
}
