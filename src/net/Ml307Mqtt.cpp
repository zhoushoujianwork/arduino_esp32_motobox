/*
 * ml307 MQTT 客户端实现
 */

#include "Ml307Mqtt.h"

Ml307Mqtt ml307Mqtt(ml307);

Ml307Mqtt::Ml307Mqtt(Ml307AtModem& modem)
    : _modem(modem)
    , _connected(false) 
    , _debug(false) {
}

/**
 * 连接MQTT服务器，参考ML307风格
 * /MQTTSTATE 只要不是3都算连接
 */
bool Ml307Mqtt::connect(const char* broker, uint16_t port,
                       const char* clientId,
                       const char* username,
                       const char* password) {
    _clientId = clientId ? clientId : "";
    _username = username ? username : "";
    _password = password ? password : "";

    // 1. 检查是否已连接，已连接则结束
    if (isConnected()) {
        return true;
    } else {
        // 如果未连接，也要确保状态为未连接
        for (int i = 0; i < 10; ++i) {
            String resp = _modem.sendATWithResponse("AT+MQTTSTATE=0");
            int s = -1;
            int idx2 = resp.indexOf("+MQTTSTATE: ");
            if (idx2 >= 0) s = resp.substring(idx2 + 12, idx2 + 13).toInt();
            if (s == 3) break;
            delay(500);
        }
    }

    // 2. 配置SSL
    if (port == 8883) {
        if (!_modem.sendATWithRetry("AT+MQTTCFG=\"ssl\",0,1")) {
            debugPrint("设置SSL失败");
            return false;
        }
    }

    // 3. 配置clean session
    if (!_modem.sendATWithRetry("AT+MQTTCFG=\"clean\",0,1")) {
        debugPrint("设置clean session失败");
        return false;
    }

    // 4. 配置keep alive
    if (!_modem.sendATWithRetry("AT+MQTTCFG=\"pingreq\",0,60")) {
        debugPrint("设置keep alive失败");
        return false;
    }

    // 5. 配置HEX编码
    if (!_modem.sendATWithRetry("AT+MQTTCFG=\"encoding\",0,1,1")) {
        debugPrint("设置HEX编码失败");
        return false;
    }

    // 6. 创建MQTT连接
    String cmd = String("AT+MQTTCONN=0,\"") + broker + "\"," + String(port) +
                 ",\"" + _clientId + "\",\"" + _username + "\",\"" + _password + "\"";
    if (!_modem.sendATWithRetry(cmd, "OK", 5, 10000)) {
        debugPrint("MQTT连接指令失败");
        return false;
    }

    // 7. 等待连接完成（最多10秒，每500ms检查一次）
    bool connected = false;
    for (int i = 0; i < 20; ++i) {
        String resp = _modem.sendATWithResponse("AT+MQTTSTATE=0");
        int s = -1;
        int idx2 = resp.indexOf("+MQTTSTATE: ");
        if (idx2 >= 0) s = resp.substring(idx2 + 12, idx2 + 13).toInt();
        if (s != 3 && s != -1) { // 只要不是3就算已连接
            connected = true;
            break;
        }
        delay(500);
    }
    if (!connected) {
        debugPrint("MQTT连接未建立");
        return false;
    }

    _connected = true;
    return true;
}

/**
 * 检查MQTT连接状态
 * 只要不是3（断开）就算连接
 */
bool Ml307Mqtt::isConnected() {
    // 查询连接状态
    String resp = _modem.sendATWithResponse("AT+MQTTSTATE=0");
    // 只要不是3（断开）就算连接
    if (resp.indexOf("+MQTTSTATE: 0") >= 0 && resp.indexOf(",3") < 0) {
        return true;
    }
    return false;
}

/**
 * 断开MQTT连接并等待状态变为未连接
 */
void Ml307Mqtt::disconnectAndWait() {
    _modem.sendAT("AT+MQTTDISC=0");
    // 最多等待5秒，每500ms检查一次
    for (int i = 0; i < 10; ++i) {
        delay(500);
        String resp = _modem.sendATWithResponse("AT+MQTTSTATE=0");
        if (resp.indexOf("+MQTTSTATE: 3") >= 0) {
            debugPrint("MQTT已断开");
            break;
        }
    }
    _connected = false;
}

/**
 * 发布消息，带mqtt_id，HEX编码
 */
bool Ml307Mqtt::publish(const char* topic, const char* payload, bool retain) {
    // String resp = _modem.sendATWithResponse("AT+MQTTSTATE=0");
    // int state = -1;
    // int idx = resp.indexOf("+MQTTSTATE: ");
    // if (idx >= 0) {
    //     state = resp.substring(idx + 12, idx + 13).toInt();
    // }
    // debugPrint("publish前MQTTSTATE: " + String(state));
    // if (state == 3) {
    //     debugPrint("MQTT未处于已连接状态，不能发布");
    //     return false;
    // }
    int qos = 0; // 可根据需要调整
    int dup = 0;
    int messageId = 0;
    String hexPayload = encodeHex(payload);
    String cmd = "AT+MQTTPUB=0,\"" + String(topic) + "\"," + String(qos) + "," + String(retain ? 1 : 0) + "," + String(dup) + "," + String(strlen(payload)) + "," + hexPayload;
    return _modem.sendAT(cmd);
}

/**
 * 订阅主题，带mqtt_id
 */
bool Ml307Mqtt::subscribe(const char* topic, uint8_t qos) {
    if (!isConnected()) return false;
    String cmd = "AT+MQTTSUB=0,\"" + String(topic) + "\"," + String(qos);
    return _modem.sendAT(cmd);
}

/**
 * 取消订阅，带mqtt_id
 */
bool Ml307Mqtt::unsubscribe(const char* topic) {
    if (!isConnected()) return false;
    String cmd = "AT+MQTTUNSUB=0,\"" + String(topic) + "\"";
    return _modem.sendAT(cmd);
}

/**
 * HEX编码工具
 */
String Ml307Mqtt::encodeHex(const char* payload) {
    String hex;
    for (size_t i = 0; i < strlen(payload); ++i) {
        char buf[3];
        sprintf(buf, "%02X", (unsigned char)payload[i]);
        hex += buf;
    }
    return hex;
}

void Ml307Mqtt::onMessage(MqttMessageCallback callback) {
    _messageCallback = callback;
}

bool Ml307Mqtt::connected() {
    return _connected;
}

// 4G 网络连接状态检查,
void Ml307Mqtt::loop() {
    if (!_connected) return;

    // 检查是否有新消息
    String response = _modem.sendATWithResponse("AT+MQTTREAD?");
    if (response.indexOf("+MQTTREAD:") >= 0) {
        parseMessageData(response);
    }
}

bool Ml307Mqtt::configureConnection() {
    // 配置客户端ID
    String cmd = "AT+MQTTCFG=\"";
    cmd += _clientId;
    cmd += "\"";
    
    return _modem.sendAT(cmd);
}

bool Ml307Mqtt::parseMessageData(const String& data) {
    // 解析 +MQTTREAD: "topic","payload" 格式的数据
    int topicStart = data.indexOf("+MQTTREAD: \"");
    if (topicStart < 0) return false;
    
    topicStart += 11;
    int topicEnd = data.indexOf("\",\"", topicStart);
    if (topicEnd < 0) return false;
    
    String topic = data.substring(topicStart, topicEnd);
    
    int payloadStart = topicEnd + 3;
    int payloadEnd = data.indexOf("\"", payloadStart);
    if (payloadEnd < 0) return false;
    
    String payload = data.substring(payloadStart, payloadEnd);
    
    if (_messageCallback) {
        _messageCallback(topic, payload);
    }
    
    return true;
}

void Ml307Mqtt::handleMessage(const String& topic, const String& payload) {
    if (_messageCallback) {
        _messageCallback(topic, payload);
    }
} 

void Ml307Mqtt::debugPrint(const String& msg) {
    if (_debug) {
        Serial.println("[ml307] [debug] " + msg);
    }
}

void Ml307Mqtt::setDebug(bool debug) {
    _debug = debug;
}