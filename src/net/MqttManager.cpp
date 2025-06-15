/*
 * MQTT 管理器实现
 */

#include "MqttManager.h"

MqttManager mqttManager;
MqttMessageCallback MqttManager::_messageCallback = nullptr;

MqttManager::MqttManager() 
    : _wifiMqttClient(nullptr)
    , _isInitialized(false)
    , _lastReconnectAttempt(0)
    , _debug(false)
    , _connectCallback(nullptr)
    , _networkState(NetworkState::DISCONNECTED)
    , _connectionStartTime(0)
    , _connectionTimeout(30000) { // 默认30秒超时
}

MqttManager::~MqttManager() {
    cleanup();
}

void MqttManager::cleanup() {
    if (_wifiMqttClient) {
        delete _wifiMqttClient;
        _wifiMqttClient = nullptr;
    }
    _isInitialized = false;
}

bool MqttManager::begin(const MqttManagerConfig& config) {
    cleanup();
    _config = config;
    // 根据配置选择网络方式
    bool result = false;
    #ifdef ENABLE_WIFI
            result = initWifi();
    #else
        result = initCellular();
    #endif

    debugPrint("MqttManager begin result: " + String(result));
    
    _isInitialized = result;

    return result;
}

bool MqttManager::initWifi() {
    if (!_config.wifiSsid || !_config.wifiPassword) {
        debugPrint("WiFi credentials not set");
        return false;
    }
    
    // 开始WiFi连接
    WiFi.begin(_config.wifiSsid, _config.wifiPassword);
    _networkState = NetworkState::CONNECTING;
    _connectionStartTime = millis();
    
    // 创建 MQTT 客户端
    _wifiMqttClient = new PubSubClient(_wifiClient);
    _wifiMqttClient->setServer(_config.broker, _config.port);
    _wifiMqttClient->setCallback(wifiMqttCallback);
    _wifiMqttClient->setKeepAlive(_config.keepAlive);
    
    return true;
}

bool MqttManager::initCellular() {
    debugPrint("MqttManager initCellular");
    _networkState = NetworkState::CONNECTING;
    _connectionStartTime = millis();
    return true;
}

bool MqttManager::connectWifi(uint32_t timeout) {
    WiFi.begin(_config.wifiSsid, _config.wifiPassword);
    
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > timeout) {
            return false;
        }
        delay(500);
    }
    
    debugPrint("WiFi connected: " + WiFi.localIP().toString());
    return true;
}

void MqttManager::disconnectWifi() {
    WiFi.disconnect();
}

bool MqttManager::connect() {
    if (!_isInitialized) {
        debugPrint("Not initialized");
        return false;
    }
    
    bool success = false;
    #ifdef ENABLE_WIFI
        debugPrint("MqttManager connect WiFi");
        if (!_wifiMqttClient) return false;
        
        success = _wifiMqttClient->connect(
            _config.clientId,
            _config.username,
            _config.password,
            nullptr,    // will topic
            0,         // will qos
            false,     // will retain
            nullptr,   // will message
            _config.cleanSession
        );
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
            _config.password
        );
    #endif

    debugPrint("MqttManager connect result: " + String(success));
    
    if (_connectCallback) {
        _connectCallback(success);
    }
    return success;
}

bool MqttManager::reconnect() {
    unsigned long now = millis();
    if (now - _lastReconnectAttempt < RECONNECT_INTERVAL) {
        return false;
    }
    _lastReconnectAttempt = now;
    
    return connect();
}

// bool MqttManager::isConnected() {
//     return _networkState == NetworkState::CONNECTED;
// }

void MqttManager::loop() {
    if (!_isInitialized) return;

    // 处理网络连接状态
    switch (_networkState) {
        case NetworkState::CONNECTING: {
            #ifdef ENABLE_WIFI
                if (WiFi.status() == WL_CONNECTED) {
                    _networkState = NetworkState::CONNECTED;
                    debugPrint("WiFi connected: " + WiFi.localIP().toString());
                    if (_networkStateCallback) {
                        _networkStateCallback(_networkState);
                    }
                } else if (millis() - _connectionStartTime > _connectionTimeout) {
                    _networkState = NetworkState::ERROR;
                    debugPrint("WiFi connection timeout");
                    if (_networkStateCallback) {
                        _networkStateCallback(_networkState);
                    }
                }
            #else
                // 4G 连接状态检查
                // if (ml307.isNetworkReady()) {
                //     _networkState = NetworkState::CONNECTED;
                //     debugPrint("Cellular connected, CSQ: " + String(ml307.getCSQ()));
                //     if (_networkStateCallback) {
                //         _networkStateCallback(_networkState);
                //     }
                // } else if (millis() - _connectionStartTime > _connectionTimeout) {
                //     _networkState = NetworkState::ERROR;
                //     debugPrint("Cellular connection timeout");
                //     if (_networkStateCallback) {
                //         _networkStateCallback(_networkState);
                //     }
                // }
            #endif
            break;
        }
        
        case NetworkState::CONNECTED: {
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
                if (_wifiMqttClient && !_wifiMqttClient->connected()) {
                    reconnect();
                }
                if (_wifiMqttClient) {
                    _wifiMqttClient->loop();
                }
            #else
                debugPrint("MqttManager loop 4G");
                ml307Mqtt.loop();
            #endif
            break;
        }
        
        case NetworkState::DISCONNECTED: {
            // 尝试重新连接
            unsigned long now = millis();
            if (now - _lastReconnectAttempt > RECONNECT_INTERVAL) {
                _lastReconnectAttempt = now;
                _networkState = NetworkState::CONNECTING;
                _connectionStartTime = now;
                
                #ifdef ENABLE_WIFI
                    WiFi.begin(_config.wifiSsid, _config.wifiPassword);
                #endif
                // 4G模式下，会自动尝试重连
            }
            break;
        }
        
        case NetworkState::ERROR: {
            // 错误状态下等待一段时间后重试
            if (millis() - _lastReconnectAttempt > RECONNECT_INTERVAL * 2) {
                _networkState = NetworkState::DISCONNECTED;
            }
            break;
        }
    }

    // 处理预注册的主题
    for (auto& topic : _topicConfigs) {
        if (topic.second.lastPublishTime + topic.second.interval < millis()) {
            if (topic.first == "device_info") {
                publishToTopic(topic.first, device_state_to_json(get_device_state()).c_str(), false);
            } else if (topic.first == "imu") {
                publishToTopic(topic.first, imu_data_to_json(imu_data).c_str(), false);
            } else if (topic.first == "gps") {
                publishToTopic(topic.first, gps_data_to_json(gps_data).c_str(), false);
            }
            // 你可以继续添加更多主题类型
        }
    }

}

bool MqttManager::publish(const char* topic, const char* payload, bool retain) {
    Serial.printf("[%lu] [MqttManager] 发布主题: %s, 内容: %s, 保留: %d\n", millis(), topic, payload, retain);
    #ifdef ENABLE_WIFI
        if (!_wifiMqttClient) {
            debugPrint("MQTT client is null");
            return false;
        }
        bool result = _wifiMqttClient->publish(topic, payload, retain);
        if (!result) {
            debugPrint("WiFi MQTT publish failed - Topic: " + String(topic));
        }
        return result;
    #else
        bool result = ml307Mqtt.publish(topic, payload, retain);
        if (!result) {
            debugPrint("Cellular MQTT publish failed - Topic: " + String(topic));
        }
        return result;
    #endif
}

bool MqttManager::subscribe(const char* topic, uint8_t qos) {
    Serial.printf("[MqttManager] 订阅主题: %s, 质量: %d\n", topic, qos);
    #ifdef ENABLE_WIFI
        return _wifiMqttClient && _wifiMqttClient->subscribe(topic, qos);
    #else
        return ml307Mqtt.subscribe(topic, qos);
    #endif
}

bool MqttManager::unsubscribe(const char* topic) {
    Serial.printf("[MqttManager] 取消订阅主题: %s\n", topic);
    #ifdef ENABLE_WIFI
        return _wifiMqttClient && _wifiMqttClient->unsubscribe(topic);
    #else
        return ml307Mqtt.unsubscribe(topic);
    #endif
}

void MqttManager::onMessage(MqttMessageCallback callback) {
    _messageCallback = callback;
    #ifdef ENABLE_GSM
        ml307Mqtt.onMessage(callback);
    #endif
}

void MqttManager::onConnect(std::function<void(bool)> callback) {
    _connectCallback = callback;
}

bool MqttManager::isNetworkConnected() const {
    #ifdef ENABLE_WIFI
        return WiFi.status() == WL_CONNECTED;
    #else
        return ml307Mqtt.isConnected();
    #endif
    return false;
}

String MqttManager::getNetworkInfo() const {
    String info;
    #ifdef ENABLE_WIFI
        info = "WiFi SSID: " + String(_config.wifiSsid);
        info += ", IP: " + WiFi.localIP().toString();
        info += ", RSSI: " + String(WiFi.RSSI());
    #else
        info = "Cellular CSQ: " + String(ml307.getCSQ());
        info += ", IP: " + ml307.getLocalIP();
    #endif
    return info;
}

void MqttManager::debugPrint(const String& msg) {
    if (_debug) {
        Serial.println("[MqttManager] [debug]:  " + msg);
    }
}

void MqttManager::wifiMqttCallback(char* topic, byte* payload, unsigned int length) {
    if (_messageCallback) {
        String payloadStr;
        payloadStr.reserve(length);
        for (unsigned int i = 0; i < length; i++) {
            payloadStr += (char)payload[i];
        }
        _messageCallback(topic, payloadStr);
    }
}

void MqttManager::onNetworkState(std::function<void(NetworkState)> callback) {
    _networkStateCallback = callback;
}

void MqttManager::addTopic(const String& name, const String& topic, unsigned long interval) {
    TopicConfig config = {
        .topic = topic,
        .interval = interval,
        .lastPublishTime = 0
    };
    _topicConfigs[name] = config;
    Serial.printf("[MqttManager] 添加主题: %s, 主题: %s, 间隔: %d\n", name.c_str(), topic.c_str(), interval);
}

bool MqttManager::publishToTopic(const String& name, const char* payload, bool retain) {
    auto it = _topicConfigs.find(name);
    if (it == _topicConfigs.end()) {
        Serial.printf("[MqttManager] 主题未找到: %s\n", name.c_str());
        return false;
    }
    
    // 检查MQTT连接状态
    #ifdef ENABLE_WIFI
        if (!_wifiMqttClient) {
            Serial.println("[MqttManager] 客户端未初始化");
            return false;
        }
        if (!_wifiMqttClient->connected()) {
            Serial.println("[MqttManager] 客户端未连接");
            return false;
        }
    #else
        if (!ml307Mqtt.connected()) {
            Serial.println("[MqttManager] 4G MQTT未连接");
            return false;
        }
    #endif
    
    unsigned long now = millis();
    if (now - it->second.lastPublishTime < it->second.interval) {
        return true; // 未到发布时间，跳过
    }
    
    bool result = publish(it->second.topic.c_str(), payload, retain);
    if (result) {
        it->second.lastPublishTime = now;
        Serial.printf("[MqttManager] 发布成功 - 主题: %s\n", it->second.topic.c_str());
    } else {
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
