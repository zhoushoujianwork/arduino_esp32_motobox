/*
 * ML370 MQTT 客户端实现
 */

#include "Ml370Mqtt.h"

Ml370Mqtt ml370Mqtt(ml370);

Ml370Mqtt::Ml370Mqtt(Ml370AtModem& modem)
    : _modem(modem)
    , _connected(false) {
}

bool Ml370Mqtt::connect(const char* broker, uint16_t port,
                       const char* clientId,
                       const char* username,
                       const char* password) {
    _clientId = clientId;
    _username = username ? username : "";
    _password = password ? password : "";
    
    // 配置 MQTT 参数
    if (!configureConnection()) {
        return false;
    }
    
    // 设置服务器信息
    String cmd = "AT+MQTTCONN=\"";
    cmd += broker;
    cmd += "\",";
    cmd += port;
    
    if (_username.length() > 0) {
        cmd += ",\"" + _username + "\",\"" + _password + "\"";
    }
    
    // 连接服务器
    if (!_modem.sendAT(cmd, "OK", 10000)) {
        return false;
    }
    
    _connected = true;
    return true;
}

void Ml370Mqtt::disconnect() {
    if (_connected) {
        _modem.sendAT("AT+MQTTDISC");
        _connected = false;
    }
}

bool Ml370Mqtt::publish(const char* topic, const char* payload, bool retain) {
    if (!_connected) return false;
    
    // 发送主题和数据
    String cmd = "AT+MQTTPUB=\"";
    cmd += topic;
    cmd += "\",\"";
    cmd += payload;
    cmd += "\"";
    if (retain) {
        cmd += ",1";
    }
    
    return _modem.sendAT(cmd);
}

bool Ml370Mqtt::subscribe(const char* topic, uint8_t qos) {
    if (!_connected) return false;
    
    String cmd = "AT+MQTTSUB=\"";
    cmd += topic;
    cmd += "\",";
    cmd += qos;
    
    return _modem.sendAT(cmd);
}

bool Ml370Mqtt::unsubscribe(const char* topic) {
    if (!_connected) return false;
    
    String cmd = "AT+MQTTUNSUB=\"";
    cmd += topic;
    cmd += "\"";
    
    return _modem.sendAT(cmd);
}

void Ml370Mqtt::onMessage(MqttMessageCallback callback) {
    _messageCallback = callback;
}

bool Ml370Mqtt::connected() {
    return _connected;
}

void Ml370Mqtt::loop() {
    if (!_connected) return;
    
    // 检查是否有新消息
    String response = _modem.sendATWithResponse("AT+MQTTREAD?");
    if (response.indexOf("+MQTTREAD:") >= 0) {
        parseMessageData(response);
    }
}

bool Ml370Mqtt::configureConnection() {
    // 配置客户端ID
    String cmd = "AT+MQTTCFG=\"";
    cmd += _clientId;
    cmd += "\"";
    
    return _modem.sendAT(cmd);
}

bool Ml370Mqtt::parseMessageData(const String& data) {
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

void Ml370Mqtt::handleMessage(const String& topic, const String& payload) {
    if (_messageCallback) {
        _messageCallback(topic, payload);
    }
} 