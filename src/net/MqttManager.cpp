/*
 * MQTT 管理器实现
 */

#include "MqttManager.h"

// 根据配置包含相应的GSM模块实例
#ifdef USE_AIR780EG_GSM
extern Air780EGModem air780eg_modem;  // 在Air780EGModem.cpp中定义
#elif defined(USE_ML307_GSM)
extern Ml307AtModem ml307_at;  // 在Ml307AtModem.cpp中定义
extern Ml307Mqtt ml307Mqtt;   // ML307 MQTT客户端
#endif

MqttManager mqttManager;
MqttMessageCallback MqttManager::_messageCallback = nullptr;

MqttManager::MqttManager()
    : _wifiMqttClient(nullptr), _isInitialized(false), _lastReconnectAttempt(0),
      _debug(false),
      _connectCallback(nullptr), _MqttState(MqttState::DISCONNECTED),
      _connectionStartTime(0), _connectionTimeout(30000),
      _lastConnectionState(false)
#ifdef USE_AIR780EG_GSM
      , _air780egMqtt(nullptr)
#endif
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
    
#ifdef USE_AIR780EG_GSM
    if (_air780egMqtt)
    {
        delete _air780egMqtt;
        _air780egMqtt = nullptr;
    }
#endif
    
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
    _wifiMqttClient->setServer(_config.broker.c_str(), _config.port);
    _wifiMqttClient->setCallback(wifiMqttCallback);
    _wifiMqttClient->setKeepAlive(_config.keepAlive);

    _connectionStartTime = millis();
    return true;
}

bool MqttManager::initCellular()
{
    debugPrint("MqttManager initCellular");
    _MqttState = MqttState::DISCONNECTED;  // 初始状态设为DISCONNECTED
    _connectionStartTime = millis();

#ifdef USE_AIR780EG_GSM
    debugPrint("初始化Air780EG MQTT模块");
    
    // 不检查网络状态，允许初始化继续
    // 网络连接将在后台任务中完成，MQTT会在网络就绪后自动连接
    debugPrint("Air780EG MQTT客户端准备就绪，等待网络连接...");
    
    // 创建Air780EG MQTT客户端，传递modem引用
    _air780egMqtt = new Air780EGMqtt(air780eg_modem);
    if (!_air780egMqtt) {
        debugPrint("Air780EG MQTT客户端创建失败");
        return false;
    }
    
    // 初始化MQTT客户端
    if (!_air780egMqtt->begin()) {
        debugPrint("Air780EG MQTT客户端初始化失败");
        delete _air780egMqtt;
        _air780egMqtt = nullptr;
        return false;
    }
    
    // 立即设置回调函数，确保消息处理正常
    if (_messageCallback) {
        debugPrint("Air780EG MQTT: 初始化时发现已有回调，立即设置桥接");
        _air780egMqtt->setCallback(air780egMqttMessageBridge);
        debugPrint("Air780EG MQTT: 初始化桥接回调设置完成");
    } else {
        debugPrint("Air780EG MQTT: 初始化时无回调函数，稍后通过onMessage设置");
    }
    
    debugPrint("Air780EG MQTT初始化成功，等待网络就绪后连接");
    return true;
    
#elif defined(USE_ML307_GSM)
    debugPrint("初始化ML307模块");
    // ML307初始化代码保持不变
    return true;
    
#else
    debugPrint("未定义GSM模块类型");
    return false;
#endif
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
        _config.clientId.c_str(),
        _config.username.c_str(),
        _config.password.c_str(),
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
    
#ifdef USE_AIR780EG_GSM
    if (_air780egMqtt) {
        success = _air780egMqtt->connect(
            _config.broker,
            _config.port,
            _config.clientId,
            _config.username,
            _config.password);
    } else {
        debugPrint("Air780EG MQTT客户端未初始化");
        success = false;
    }
#elif defined(USE_ML307_GSM)
    success = ml307Mqtt.connect(
        _config.broker,
        _config.port,
        _config.clientId,
        _config.username,
        _config.password);
#else
    debugPrint("未定义GSM模块类型");
    success = false;
#endif
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

bool MqttManager::forceReconnect()
{
    debugPrint("MqttManager 强制重新连接");

    // 先断开现有连接
#ifdef ENABLE_WIFI
    if (_wifiMqttClient && _wifiMqttClient->connected())
    {
        _wifiMqttClient->disconnect();
        delay(100); // 等待断开完成
    }
#else
#ifdef USE_ML307_GSM
    if (ml307Mqtt.connected())
    {
        ml307Mqtt.disconnect();
        delay(500); // 4G模块需要更长时间断开
    }
#endif
#endif

    // 重置连接状态
    setMqttState(MqttState::CONNECTING);
    _lastReconnectAttempt = millis();
    _connectionStartTime = millis();

    // 尝试重新连接
    return connect();
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

bool MqttManager::isConnected() {
    return _MqttState == MqttState::CONNECTED;
}

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

    // 检查间隔 - 每1秒检查一次
    if (millis() - _lastNetworkCheckTime < 1000)
        return;
        
    _lastNetworkCheckTime = millis();

#ifdef ENABLE_GSM
    debugPrint("=== MqttManager::loop 开始 ===");
    
#ifdef USE_AIR780EG_GSM
    // 检查网络状态
    bool networkReady = air780eg_modem.isNetworkReadyCheck();
    debugPrint("Air780EG网络状态: " + String(networkReady ? "就绪" : "未就绪"));
    
    if (!networkReady) {
        debugPrint("网络未就绪，跳过MQTT处理");
        setMqttState(MqttState::DISCONNECTED);
        return;
    }
    
    // 网络就绪，确保MQTT客户端存在并处理消息
    if (_air780egMqtt) {
        debugPrint("Air780EG MQTT客户端存在，调用loop()");
        bool mqttConnected = _air780egMqtt->isConnected();
        debugPrint("Air780EG MQTT连接状态: " + String(mqttConnected ? "已连接" : "未连接"));
        
        // 无论连接状态如何，都调用loop()来处理消息和维护连接
        _air780egMqtt->loop();
        
        if (mqttConnected) {
            setMqttState(MqttState::CONNECTED);
            
            // 检查连接状态变化并触发回调
            if (!_lastConnectionState) {
                _lastConnectionState = true;
                if (_connectCallback) {
                    debugPrint("触发MQTT连接成功回调");
                    _connectCallback(true);
                }
            }
        } else {
            setMqttState(MqttState::DISCONNECTED);
            
            // 检查连接状态变化并触发回调
            if (_lastConnectionState) {
                _lastConnectionState = false;
                if (_connectCallback) {
                    debugPrint("触发MQTT连接断开回调");
                    _connectCallback(false);
                }
            }
            
            // 尝试重连 - 使用静态变量避免重复声明
            static unsigned long lastReconnectAttempt = 0;
            if (millis() - lastReconnectAttempt > 15000) { // 15秒重连一次
                lastReconnectAttempt = millis();
                debugPrint("尝试重新连接MQTT...");
                connect();
            }
        }
    } else {
        debugPrint("❌ Air780EG MQTT客户端不存在");
        setMqttState(MqttState::ERROR);
    }
    
#elif defined(USE_ML307_GSM)
    // ML307逻辑保持不变
    if (!ml307_at.isNetworkReadyCheck()) {
        setMqttState(MqttState::DISCONNECTED);
        return;
    }
    
    if (ml307_at.isLBSLoading())
        return;
        
    ml307Mqtt.loop();
    
    bool currentConnected = ml307Mqtt.connected();
    if (currentConnected != _lastConnectionState) {
        _lastConnectionState = currentConnected;
        if (_connectCallback) {
            _connectCallback(currentConnected);
        }
    }
    
    if (currentConnected) {
        setMqttState(MqttState::CONNECTED);
    } else {
        setMqttState(MqttState::DISCONNECTED);
        static unsigned long lastReconnectAttempt = 0;
        if (millis() - lastReconnectAttempt > 15000) {
            lastReconnectAttempt = millis();
            connect();
        }
    }
#endif

    debugPrint("=== MqttManager::loop 结束 ===");

#else
    // WiFi逻辑简化
    bool networkConnected = isNetworkConnected();
    bool mqttConnected = isMqttConnected();
    
    if (!networkConnected) {
        setMqttState(MqttState::DISCONNECTED);
        static unsigned long lastWifiReconnect = 0;
        if (millis() - lastWifiReconnect > 10000) {
            lastWifiReconnect = millis();
            connectWifi();
        }
    } else if (!mqttConnected) {
        setMqttState(MqttState::CONNECTING);
        reconnect();
    } else {
        setMqttState(MqttState::CONNECTED);
    }
    
    // 检查连接状态变化
    bool currentConnected = networkConnected && mqttConnected;
    if (currentConnected != _lastConnectionState) {
        _lastConnectionState = currentConnected;
        if (_connectCallback) {
            _connectCallback(currentConnected);
        }
    }
    
    if (_wifiMqttClient)
        _wifiMqttClient->loop();
#endif

    // 处理预注册的主题 - 添加非阻塞检查
    static unsigned long lastTopicCheck = 0;
    if (millis() - lastTopicCheck > 1000) { // 每1秒检查一次主题发布
        lastTopicCheck = millis();
        
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
    }
}

bool MqttManager::publish(const char *topic, const char *payload, bool retain)
{
    if (!topic || !payload)
    {
        debugPrint("MQTT publish 参数无效 - topic或payload为空");
        return false;
    }

    debugPrint("MqttManager publish topic: " + String(topic) + ", payload长度: " + String(strlen(payload)) + ", retain: " + String(retain));

    // 发布前再次检查连接状态
    if (!isNetworkConnected())
    {
        debugPrint("MQTT publish 失败 - 网络未连接");
        setMqttState(MqttState::DISCONNECTED);
        return false;
    }

    if (!isMqttConnected())
    {
        debugPrint("MQTT publish 失败 - MQTT未连接");
        setMqttState(MqttState::CONNECTING);
        return false;
    }

#ifdef ENABLE_WIFI
    if (!_wifiMqttClient)
    {
        debugPrint("MQTT client is null");
        return false;
    }
    bool result = _wifiMqttClient->publish(topic, payload, retain);
    if (!result)
    {
        debugPrint("WiFi MQTT publish 失败 - Topic: " + String(topic) + ", 客户端状态: " + String(_wifiMqttClient->state()));
    }
    return result;
#else
#ifdef USE_AIR780EG_GSM
    if (_air780egMqtt) {
        bool result = _air780egMqtt->publish(String(topic), String(payload), 0);
        if (!result) {
            debugPrint("Air780EG MQTT publish 失败 - Topic: " + String(topic));
        }
        return result;
    } else {
        debugPrint("Air780EG MQTT客户端未初始化");
        return false;
    }
#elif defined(USE_ML307_GSM)
    bool result = ml307Mqtt.publish(topic, payload, retain);
    if (!result) {
        debugPrint("ML307 MQTT publish 失败 - Topic: " + String(topic));
    }
    return result;
#else
    debugPrint("未定义GSM模块类型");
    return false;
#endif
#endif
}

bool MqttManager::subscribe(const char *topic, uint8_t qos)
{
    debugPrint("=== MqttManager::subscribe 开始 ===");
    debugPrint("MqttManager: 订阅请求 - 主题: " + String(topic));
    debugPrint("MqttManager: 订阅请求 - QoS: " + String(qos));
    Serial.printf("[MqttManager] 订阅主题: %s, 质量: %d\n", topic, qos);
    
#ifdef ENABLE_WIFI
    debugPrint("MqttManager: 使用WiFi网络进行订阅");
    if (_wifiMqttClient) {
        debugPrint("MqttManager: WiFi MQTT客户端存在");
        bool result = _wifiMqttClient->subscribe(topic, qos);
        debugPrint("MqttManager: WiFi订阅结果: " + String(result ? "成功" : "失败"));
        debugPrint("=== MqttManager::subscribe 结束 (WiFi) ===");
        return result;
    } else {
        debugPrint("MqttManager: ❌ WiFi MQTT客户端不存在");
        debugPrint("=== MqttManager::subscribe 结束 (WiFi客户端不存在) ===");
        return false;
    }
#else
#ifdef USE_AIR780EG_GSM
    debugPrint("MqttManager: 使用Air780EG GSM网络进行订阅");
    if (_air780egMqtt) {
        debugPrint("MqttManager: Air780EG MQTT客户端存在");
        debugPrint("MqttManager: Air780EG连接状态: " + String(_air780egMqtt->isConnected() ? "已连接" : "未连接"));
        debugPrint("MqttManager: 调用Air780EG订阅函数...");
        bool result = _air780egMqtt->subscribe(String(topic), qos);
        debugPrint("MqttManager: Air780EG订阅结果: " + String(result ? "成功" : "失败"));
        debugPrint("=== MqttManager::subscribe 结束 (Air780EG) ===");
        return result;
    } else {
        debugPrint("MqttManager: ❌ Air780EG MQTT客户端未初始化");
        debugPrint("=== MqttManager::subscribe 结束 (Air780EG未初始化) ===");
        return false;
    }
#elif defined(USE_ML307_GSM)
    debugPrint("MqttManager: 使用ML307 GSM网络进行订阅");
    bool result = ml307Mqtt.subscribe(topic, qos);
    debugPrint("MqttManager: ML307订阅结果: " + String(result ? "成功" : "失败"));
    debugPrint("=== MqttManager::subscribe 结束 (ML307) ===");
    return result;
#else
    debugPrint("MqttManager: ❌ 未定义GSM模块类型");
    debugPrint("=== MqttManager::subscribe 结束 (未定义GSM) ===");
    return false;
#endif
#endif
}

bool MqttManager::unsubscribe(const char *topic)
{
    Serial.printf("[MqttManager] 取消订阅主题: %s\n", topic);
#ifdef ENABLE_WIFI
    return _wifiMqttClient && _wifiMqttClient->unsubscribe(topic);
#else
#ifdef USE_AIR780EG_GSM
    if (_air780egMqtt) {
        return _air780egMqtt->unsubscribe(String(topic));
    } else {
        debugPrint("Air780EG MQTT客户端未初始化");
        return false;
    }
#elif defined(USE_ML307_GSM)
    return ml307Mqtt.unsubscribe(topic);
#else
    debugPrint("未定义GSM模块类型");
    return false;
#endif
#endif
}

void MqttManager::onMessage(MqttMessageCallback callback)
{
    debugPrint("=== MqttManager::onMessage 开始设置回调 ===");
    _messageCallback = callback;
    debugPrint("MqttManager: 回调函数已存储，地址: " + String((unsigned long)&callback, HEX));
    
#ifdef ENABLE_GSM
#ifdef USE_AIR780EG_GSM
    debugPrint("MqttManager: 检测到Air780EG GSM配置");
    // 为Air780EG设置静态回调桥接函数
    if (_air780egMqtt) {
        debugPrint("MqttManager: Air780EG MQTT客户端存在，设置桥接回调");
        _air780egMqtt->setCallback(air780egMqttMessageBridge);
        debugPrint("MqttManager: Air780EG桥接回调设置完成");
    } else {
        debugPrint("MqttManager: 警告 - Air780EG MQTT客户端为空，无法设置回调");
    }
#elif defined(USE_ML307_GSM)
    debugPrint("MqttManager: 使用ML307 GSM，设置直接回调");
    ml307Mqtt.onMessage(callback);
#endif
#endif
    debugPrint("=== MqttManager::onMessage 回调设置完成 ===");
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
#ifdef USE_AIR780EG_GSM
    return air780eg_modem.isNetworkReady();
#elif defined(USE_ML307_GSM)
    return ml307_at.isNetworkReady();
#else
    return false;
#endif
#endif
}

String MqttManager::getNetworkInfo() const
{
    String info;
#ifdef ENABLE_WIFI
    info = "WiFi SSID: " + String(WiFi.SSID());
    info += ", IP: " + WiFi.localIP().toString();
    info += ", RSSI: " + String(WiFi.RSSI());
#else
#ifdef USE_AIR780EG_GSM
    info = "Cellular CSQ: " + String(air780eg_modem.getCSQ());
    info += ", IP: " + air780eg_modem.getLocalIP();
#elif defined(USE_ML307_GSM)
    info = "Cellular CSQ: " + String(ml307_at.getCSQ());
    info += ", IP: " + ml307_at.getLocalIP();
#endif
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

void MqttManager::testMQTTSupport() {
    debugPrint("开始测试MQTT功能支持");
    
#ifdef USE_AIR780EG_GSM
    if (_air780egMqtt) {
        debugPrint("使用Air780EG MQTT实现");
        bool result = _air780egMqtt->testMQTTSupport();
        if (result) {
            debugPrint("✅ Air780EG MQTT功能测试通过");
        } else {
            debugPrint("❌ Air780EG MQTT功能测试失败");
        }
    } else {
        debugPrint("❌ Air780EG MQTT对象未初始化");
    }
#elif defined(USE_ML307_GSM)
    if (ml307Mqtt.connected()) {
        debugPrint("使用ML307 MQTT实现");
        debugPrint("✅ ML307 MQTT连接正常");
    } else {
        debugPrint("使用ML307 MQTT实现");
        debugPrint("❌ ML307 MQTT未连接");
    }
#elif defined(ENABLE_WIFI)
    debugPrint("使用WiFi MQTT实现");
    if (_wifiClient && _wifiMqttClient) {
        debugPrint("✅ WiFi MQTT客户端已初始化");
        debugPrint("WiFi连接状态: " + String(WiFi.isConnected() ? "已连接" : "未连接"));
        if (WiFi.isConnected()) {
            debugPrint("WiFi IP: " + WiFi.localIP().toString());
            debugPrint("WiFi信号强度: " + String(WiFi.RSSI()) + " dBm");
        }
    } else {
        debugPrint("❌ WiFi MQTT客户端未初始化");
    }
#else
    debugPrint("❌ 未启用任何MQTT实现");
#endif
    
    debugPrint("MQTT测试完成");
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

#ifdef USE_AIR780EG_GSM
void MqttManager::air780egMqttMessageBridge(String topic, String payload)
{
    Serial.println("=== Air780EG MQTT 桥接回调触发 ===");
    Serial.println("桥接回调 - 主题: " + topic);
    Serial.println("桥接回调 - 负载: " + payload);
    Serial.println("桥接回调 - 负载长度: " + String(payload.length()));
    
    if (_messageCallback)
    {
        Serial.println("桥接回调 - 应用层回调存在，开始转发消息");
        Serial.println("桥接回调 - 回调函数地址: " + String((unsigned long)&_messageCallback, HEX));
        _messageCallback(topic, payload);
        Serial.println("桥接回调 - 消息转发完成");
    } else {
        Serial.println("桥接回调 - 错误：应用层回调为空，消息丢失！");
    }
    Serial.println("=== Air780EG MQTT 桥接回调结束 ===");
}
#endif

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

    // 统一的连接状态检查
    if (!isNetworkConnected() || !isMqttConnected())
    {
        debugPrint("MqttManager 网络或MQTT未连接 - 网络状态: " + String(isNetworkConnected()) + ", MQTT状态: " + String(isMqttConnected()));
        setMqttState(MqttState::DISCONNECTED);
        return false;
    }

    // 检查发布间隔
    unsigned long now = millis();
    if (now - it->second.lastPublishTime < it->second.interval)
    {
        return true; // 未到发布时间，跳过
    }

    // 执行发布操作
    bool result = publish(it->second.topic.c_str(), payload, retain);
    if (result)
    {
        it->second.lastPublishTime = now;
        Serial.println("MqttManager 发布成功 - 主题: " + it->second.topic + ", 负载长度: " + String(strlen(payload)));
    }
    else
    {
        // 发布失败时，重新检查连接状态并设置相应的状态
        debugPrint("MqttManager 发布失败 - 主题: " + it->second.topic);

        // 重新验证连接状态
        if (!isNetworkConnected())
        {
            debugPrint("MqttManager 网络连接已断开");
            setMqttState(MqttState::DISCONNECTED);
        }
        else if (!isMqttConnected())
        {
            debugPrint("MqttManager MQTT连接已断开，尝试强制重连");
            setMqttState(MqttState::CONNECTING);
            // 对于重要的主题，尝试强制重连
            if (name == "device_info" || name == "gps")
            {
                forceReconnect();
            }
        }
        else
        {
            debugPrint("MqttManager 发布失败但连接正常，可能是临时网络问题");
            setMqttState(MqttState::ERROR);
        }
    }
    return result;
}

// 添加MQTT连接状态检查方法
bool MqttManager::isMqttConnected() const
{
#ifdef ENABLE_WIFI
    return _wifiMqttClient && _wifiMqttClient->connected();
#else
#ifdef USE_AIR780EG_GSM
    if (_air780egMqtt) {
        return _air780egMqtt->isConnected();
    } else {
        return false;
    }
#elif defined(USE_ML307_GSM)
    return ml307Mqtt.connected() && ml307Mqtt.canPublish();
#else
    return false;
#endif
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
