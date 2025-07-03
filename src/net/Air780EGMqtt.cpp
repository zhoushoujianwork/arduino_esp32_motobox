/*
 * Air780EG MQTT 管理器实现
 */

#include "Air780EGMqtt.h"

Air780EGMqtt::Air780EGMqtt(Air780EGModem& modem) 
    : _modem(modem), _debug(false), _connected(false), 
      _port(1883), _messageCallback(nullptr) {
}

bool Air780EGMqtt::begin() {
    debugPrint("Air780EG MQTT: 初始化");
    
    if (!_modem.isNetworkReady()) {
        debugPrint("Air780EG MQTT: 网络未就绪");
        return false;
    }
    
    return setupMQTT();
}

bool Air780EGMqtt::setupMQTT() {
    // 配置MQTT参数
    if (!_modem.sendAT("AT+CMQTTSTART", "OK", 5000)) {
        debugPrint("Air780EG MQTT: 启动MQTT服务失败");
        return false;
    }
    
    // 获取客户端索引
    if (!_modem.sendAT("AT+CMQTTACCQ=0,\"" + _clientId + "\"", "OK", 3000)) {
        debugPrint("Air780EG MQTT: 获取客户端失败");
        return false;
    }
    
    return true;
}

bool Air780EGMqtt::connect(const String& server, int port, const String& clientId, 
                          const String& username, const String& password) {
    _server = server;
    _port = port;
    _clientId = clientId;
    _username = username;
    _password = password;
    
    debugPrint("Air780EG MQTT: 连接到 " + server + ":" + String(port));
    
    if (!_modem.isNetworkReady()) {
        debugPrint("Air780EG MQTT: 网络未就绪");
        return false;
    }
    
    // 设置MQTT服务器
    String cmd = "AT+CMQTTCONNECT=0,\"tcp://" + server + ":" + String(port) + "\",60,1";
    if (username.length() > 0) {
        cmd += ",\"" + username + "\",\"" + password + "\"";
    }
    
    if (_modem.sendAT(cmd, "OK", 10000)) {
        // 等待连接确认
        delay(2000);
        if (checkConnection()) {
            _connected = true;
            debugPrint("Air780EG MQTT: 连接成功");
            return true;
        }
    }
    
    debugPrint("Air780EG MQTT: 连接失败");
    return false;
}

bool Air780EGMqtt::disconnect() {
    debugPrint("Air780EG MQTT: 断开连接");
    
    if (_modem.sendAT("AT+CMQTTDISC=0,60", "OK", 5000)) {
        _connected = false;
        return true;
    }
    return false;
}

bool Air780EGMqtt::isConnected() {
    return _connected && checkConnection();
}

bool Air780EGMqtt::checkConnection() {
    String response = _modem.sendATWithResponse("AT+CMQTTCONNECT?", 2000);
    return response.indexOf("+CMQTTCONNECT: 0,0") >= 0;  // 0表示已连接
}

bool Air780EGMqtt::publish(const String& topic, const String& payload, int qos) {
    if (!_connected) {
        debugPrint("Air780EG MQTT: 未连接，无法发布消息");
        return false;
    }
    
    debugPrint("Air780EG MQTT: 发布到 " + topic + " -> " + payload.substring(0, 50) + "...");
    
    // Air780EG MQTT发布命令格式
    String cmd = "AT+CMQTTTOPIC=0," + String(topic.length());
    if (!_modem.sendAT(cmd, ">", 3000)) {
        return false;
    }
    
    if (!_modem.sendAT(topic, "OK", 3000)) {
        return false;
    }
    
    cmd = "AT+CMQTTPAYLOAD=0," + String(payload.length());
    if (!_modem.sendAT(cmd, ">", 3000)) {
        return false;
    }
    
    if (!_modem.sendAT(payload, "OK", 3000)) {
        return false;
    }
    
    cmd = "AT+CMQTTPUB=0," + String(qos) + ",60";
    return _modem.sendAT(cmd, "OK", 5000);
}

bool Air780EGMqtt::subscribe(const String& topic, int qos) {
    if (!_connected) {
        debugPrint("Air780EG MQTT: 未连接，无法订阅");
        return false;
    }
    
    debugPrint("Air780EG MQTT: 订阅主题 " + topic);
    
    String cmd = "AT+CMQTTTOPIC=0," + String(topic.length());
    if (!_modem.sendAT(cmd, ">", 3000)) {
        return false;
    }
    
    if (!_modem.sendAT(topic, "OK", 3000)) {
        return false;
    }
    
    cmd = "AT+CMQTTSUB=0," + String(qos) + ",60";
    return _modem.sendAT(cmd, "OK", 5000);
}

bool Air780EGMqtt::unsubscribe(const String& topic) {
    if (!_connected) {
        return false;
    }
    
    debugPrint("Air780EG MQTT: 取消订阅 " + topic);
    
    String cmd = "AT+CMQTTTOPIC=0," + String(topic.length());
    if (!_modem.sendAT(cmd, ">", 3000)) {
        return false;
    }
    
    if (!_modem.sendAT(topic, "OK", 3000)) {
        return false;
    }
    
    return _modem.sendAT("AT+CMQTTUNSUB=0,60", "OK", 5000);
}

void Air780EGMqtt::loop() {
    if (!_connected) return;
    
    // 检查是否有新消息
    String response = _modem.sendATWithResponse("AT+CMQTTMSGGET=0", 1000);
    if (response.length() > 0 && response.indexOf("+CMQTTMSGGET:") >= 0) {
        handleIncomingMessage(response);
    }
}

void Air780EGMqtt::handleIncomingMessage(const String& data) {
    // 解析MQTT消息格式
    // +CMQTTMSGGET: 0,<topic_len>,<topic>,<payload_len>,<payload>
    
    int start = data.indexOf("+CMQTTMSGGET: 0,") + 16;
    if (start < 16) return;
    
    String remaining = data.substring(start);
    int comma1 = remaining.indexOf(',');
    if (comma1 < 0) return;
    
    int topicLen = remaining.substring(0, comma1).toInt();
    remaining = remaining.substring(comma1 + 1);
    
    if (remaining.length() < topicLen) return;
    String topic = remaining.substring(0, topicLen);
    remaining = remaining.substring(topicLen + 1);
    
    int comma2 = remaining.indexOf(',');
    if (comma2 < 0) return;
    
    int payloadLen = remaining.substring(0, comma2).toInt();
    String payload = remaining.substring(comma2 + 1, comma2 + 1 + payloadLen);
    
    debugPrint("Air780EG MQTT: 收到消息 " + topic + " -> " + payload);
    
    if (_messageCallback) {
        _messageCallback(topic, payload);
    }
}

void Air780EGMqtt::setCallback(void (*callback)(String topic, String payload)) {
    _messageCallback = callback;
}

void Air780EGMqtt::setDebug(bool debug) {
    _debug = debug;
}

void Air780EGMqtt::debugPrint(const String& msg) {
    if (_debug) {
        Serial.println("[Air780EG MQTT] " + msg);
    }
}
