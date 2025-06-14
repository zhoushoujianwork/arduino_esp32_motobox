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
    switch (_config.networkType) {
        case MqttNetworkType::WIFI:
            result = initWifi();
            break;
        case MqttNetworkType::CELLULAR:
            result = initCellular();
            break;
        default:
            debugPrint("Invalid network type");
            return false;
    }
    
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
    if (!ml370.isReady()) {
        debugPrint("ML370 not ready");
        return false;
    }
    
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
    if (_config.networkType == MqttNetworkType::WIFI) {
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
    } else {
        success = ml370Mqtt.connect(
            _config.broker,
            _config.port,
            _config.clientId,
            _config.username,
            _config.password
        );
    }
    
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

bool MqttManager::isConnected() {
    return _networkState == NetworkState::CONNECTED;
}

void MqttManager::loop() {
    if (!_isInitialized) return;

    // 处理网络连接状态
    switch (_networkState) {
        case NetworkState::CONNECTING: {
            if (_config.networkType == MqttNetworkType::WIFI) {
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
            } else {
                // 4G 连接状态检查
                if (ml370.isReady() && ml370.getCSQ() > 0) {
                    _networkState = NetworkState::CONNECTED;
                    debugPrint("Cellular connected, CSQ: " + String(ml370.getCSQ()));
                    if (_networkStateCallback) {
                        _networkStateCallback(_networkState);
                    }
                } else if (millis() - _connectionStartTime > _connectionTimeout) {
                    _networkState = NetworkState::ERROR;
                    debugPrint("Cellular connection timeout");
                    if (_networkStateCallback) {
                        _networkStateCallback(_networkState);
                    }
                }
            }
            break;
        }
        
        case NetworkState::CONNECTED: {
            // 检查网络是否断开
            bool isConnected = (_config.networkType == MqttNetworkType::WIFI) 
                ? (WiFi.status() == WL_CONNECTED)
                : (ml370.isReady() && ml370.getCSQ() > 0);
                
            if (!isConnected) {
                _networkState = NetworkState::DISCONNECTED;
                debugPrint("Network disconnected");
                if (_networkStateCallback) {
                    _networkStateCallback(_networkState);
                }
            }
            
            // MQTT 相关处理
            if (_config.networkType == MqttNetworkType::WIFI) {
                if (_wifiMqttClient && !_wifiMqttClient->connected()) {
                    reconnect();
                }
                if (_wifiMqttClient) {
                    _wifiMqttClient->loop();
                }
            } else {
                ml370Mqtt.loop();
            }
            break;
        }
        
        case NetworkState::DISCONNECTED: {
            // 尝试重新连接
            unsigned long now = millis();
            if (now - _lastReconnectAttempt > RECONNECT_INTERVAL) {
                _lastReconnectAttempt = now;
                _networkState = NetworkState::CONNECTING;
                _connectionStartTime = now;
                
                if (_config.networkType == MqttNetworkType::WIFI) {
                    WiFi.begin(_config.wifiSsid, _config.wifiPassword);
                }
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

    // 处理主题发布
    
}

bool MqttManager::publish(const char* topic, const char* payload, bool retain) {
    if (_config.networkType == MqttNetworkType::WIFI) {
        if (!_wifiMqttClient) {
            debugPrint("MQTT client is null");
            return false;
        }
        bool result = _wifiMqttClient->publish(topic, payload, retain);
        if (!result) {
            debugPrint("WiFi MQTT publish failed - Topic: " + String(topic));
        }
        return result;
    } else {
        bool result = ml370Mqtt.publish(topic, payload, retain);
        if (!result) {
            debugPrint("Cellular MQTT publish failed - Topic: " + String(topic));
        }
        return result;
    }
}

bool MqttManager::subscribe(const char* topic, uint8_t qos) {
    if (_config.networkType == MqttNetworkType::WIFI) {
        return _wifiMqttClient && _wifiMqttClient->subscribe(topic, qos);
    } else {
        return ml370Mqtt.subscribe(topic, qos);
    }
}

bool MqttManager::unsubscribe(const char* topic) {
    if (_config.networkType == MqttNetworkType::WIFI) {
        return _wifiMqttClient && _wifiMqttClient->unsubscribe(topic);
    } else {
        return ml370Mqtt.unsubscribe(topic);
    }
}

void MqttManager::onMessage(MqttMessageCallback callback) {
    _messageCallback = callback;
    if (_config.networkType != MqttNetworkType::WIFI) {
        ml370Mqtt.onMessage(callback);
    }
}

void MqttManager::onConnect(std::function<void(bool)> callback) {
    _connectCallback = callback;
}

bool MqttManager::isNetworkConnected() const {
    if (_config.networkType == MqttNetworkType::WIFI) {
        return WiFi.status() == WL_CONNECTED;
    } else {
        return ml370.isReady();
    }
}

String MqttManager::getNetworkInfo() const {
    String info;
    if (_config.networkType == MqttNetworkType::WIFI) {
        info = "WiFi SSID: " + String(_config.wifiSsid);
        info += ", IP: " + WiFi.localIP().toString();
        info += ", RSSI: " + String(WiFi.RSSI());
    } else {
        info = "Cellular CSQ: " + String(ml370.getCSQ());
        info += ", IP: " + ml370.getLocalIP();
    }
    return info;
}

void MqttManager::debugPrint(const String& msg) {
    if (_debug) {
        Serial.println("MqttManager: " + msg);
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
}

bool MqttManager::publishToTopic(const String& name, const char* payload, bool retain) {
    auto it = _topicConfigs.find(name);
    if (it == _topicConfigs.end()) {
        Serial.printf("[MQTT] 主题未找到: %s\n", name.c_str());
        return false;
    }
    
    // 检查MQTT连接状态
    if (_config.networkType == MqttNetworkType::WIFI) {
        if (!_wifiMqttClient) {
            Serial.println("[MQTT] 客户端未初始化");
            return false;
        }
        if (!_wifiMqttClient->connected()) {
            Serial.println("[MQTT] 客户端未连接");
            return false;
        }
    } else {
        if (!ml370Mqtt.connected()) {
            Serial.println("[MQTT] 4G MQTT未连接");
            return false;
        }
    }
    
    unsigned long now = millis();
    if (now - it->second.lastPublishTime < it->second.interval) {
        return true; // 未到发布时间，跳过
    }
    
    bool result = publish(it->second.topic.c_str(), payload, retain);
    if (result) {
        it->second.lastPublishTime = now;
        Serial.printf("[MQTT] 发布成功 - 主题: %s\n", it->second.topic.c_str());
    } else {
        Serial.printf("[MQTT] 发布失败 - 主题: %s, 网络类型: %s\n", 
            it->second.topic.c_str(),
            _config.networkType == MqttNetworkType::WIFI ? "WiFi" : "4G");
    }
    return result;
}
