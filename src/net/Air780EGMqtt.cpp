/*
 * Air780EG MQTT 管理器实现
 */

#include "Air780EGMqtt.h"

Air780EGMqtt::Air780EGMqtt(Air780EGModem& modem) 
    : _modem(modem), _debug(true), _connected(false), 
      _port(1883), _messageCallback(nullptr) {
}

bool Air780EGMqtt::begin() {
    debugPrint("Air780EG MQTT: 初始化");
    
    if (!_modem.isNetworkReady()) {
        debugPrint("Air780EG MQTT: 网络未就绪，但允许初始化，等待网络连接后再设置MQTT服务");
        return true; // 允许初始化，稍后在connect时再设置MQTT服务
    }
    
    return setupMQTT();
}

bool Air780EGMqtt::setupMQTT() {
    debugPrint("Air780EG MQTT: 开始设置MQTT服务");
    
    // 先检查MQTT服务状态
    String response = _modem.sendATWithResponse("AT+CMQTTSTART?", 3000);
    debugPrint("Air780EG MQTT: 服务状态查询: " + response);
    
    // 如果服务已经启动，直接返回成功
    if (response.indexOf("OK") >= 0 && response.indexOf("ERROR") < 0) {
        debugPrint("Air780EG MQTT: MQTT服务可能已启动");
        return true;
    }
    
    // 尝试停止可能存在的服务（忽略错误）
    _modem.sendAT("AT+CMQTTSTOP", "OK", 3000);
    delay(1000);
    
    // 启动MQTT服务
    if (_modem.sendAT("AT+CMQTTSTART", "OK", 5000)) {
        debugPrint("Air780EG MQTT: MQTT服务启动成功");
        return true;
    } else {
        debugPrint("Air780EG MQTT: MQTT服务启动失败，可能固件不支持或网络未就绪");
        
        // 尝试查询错误信息
        String errorResponse = _modem.sendATWithResponse("AT+CMQTTSTART", 3000);
        debugPrint("Air780EG MQTT: 错误响应: " + errorResponse);
        
        return false;
    }
}

bool Air780EGMqtt::connect(const String& server, int port, const String& clientId, 
                          const String& username, const String& password) {
    _server = server;
    _port = port;
    _clientId = clientId;
    _username = username;
    _password = password;
    
    debugPrint("Air780EG MQTT: 连接到 " + server + ":" + String(port));
    debugPrint("Air780EG MQTT: 客户端ID: " + clientId);
    debugPrint("Air780EG MQTT: 用户名: " + username);
    
    if (!_modem.isNetworkReady()) {
        debugPrint("Air780EG MQTT: 网络未就绪");
        return false;
    }
    
    // 确保MQTT服务已启动
    if (!setupMQTT()) {
        debugPrint("Air780EG MQTT: MQTT服务设置失败");
        return false;
    }
    
    // 先断开可能存在的连接
    _modem.sendAT("AT+CMQTTDISC=0,60", "OK", 3000);
    delay(500);
    
    // 释放可能存在的客户端
    _modem.sendAT("AT+CMQTTREL=0", "OK", 3000);
    delay(500);
    
    // 获取客户端索引
    if (!_modem.sendAT("AT+CMQTTACCQ=0,\"" + clientId + "\"", "OK", 5000)) {
        debugPrint("Air780EG MQTT: 获取客户端失败");
        return false;
    }
    
    debugPrint("Air780EG MQTT: 客户端获取成功");
    
    // 设置MQTT服务器连接
    String cmd = "AT+CMQTTCONNECT=0,\"tcp://" + server + ":" + String(port) + "\",60,1";
    if (username.length() > 0) {
        cmd += ",\"" + username + "\",\"" + password + "\"";
    }
    
    debugPrint("Air780EG MQTT: 发送连接命令: " + cmd);
    
    if (_modem.sendAT(cmd, "OK", 15000)) {
        debugPrint("Air780EG MQTT: 连接命令发送成功，等待连接确认...");
        
        // 等待连接建立
        for (int i = 0; i < 10; i++) {
            delay(1000);
            if (checkConnection()) {
                _connected = true;
                debugPrint("Air780EG MQTT: 连接成功！");
                return true;
            }
            debugPrint("Air780EG MQTT: 等待连接... (" + String(i+1) + "/10)");
        }
        
        debugPrint("Air780EG MQTT: 连接超时");
    } else {
        debugPrint("Air780EG MQTT: 连接命令发送失败");
    }
    
    debugPrint("Air780EG MQTT: 连接失败");
    return false;
}

bool Air780EGMqtt::disconnect() {
    debugPrint("Air780EG MQTT: 断开连接");
    
    bool success = true;
    
    // 断开MQTT连接
    if (!_modem.sendAT("AT+CMQTTDISC=0,60", "OK", 5000)) {
        debugPrint("Air780EG MQTT: 断开连接失败");
        success = false;
    }
    
    // 释放客户端
    if (!_modem.sendAT("AT+CMQTTREL=0", "OK", 3000)) {
        debugPrint("Air780EG MQTT: 释放客户端失败");
        success = false;
    }
    
    _connected = false;
    
    if (success) {
        debugPrint("Air780EG MQTT: 断开连接成功");
    }
    
    return success;
}

bool Air780EGMqtt::isConnected() {
    // 如果内部状态显示未连接，直接返回false
    if (!_connected) {
        return false;
    }
    
    // 减少频繁查询，每5秒检查一次实际连接状态
    static unsigned long lastCheck = 0;
    static bool lastResult = false;
    unsigned long now = millis();
    
    if (now - lastCheck > 5000) { // 5秒检查一次
        lastCheck = now;
        lastResult = checkConnection();
        if (!lastResult) {
            _connected = false; // 如果检查失败，更新内部状态
            debugPrint("Air780EG MQTT: 连接状态检查失败，更新内部状态为未连接");
        }
    }
    
    return _connected && lastResult;
}

bool Air780EGMqtt::checkConnection() {
    // 查询MQTT连接状态
    String response = _modem.sendATWithResponse("AT+CMQTTCONNECT?", 3000);
    debugPrint("Air780EG MQTT: 连接状态查询响应: " + response);
    
    // Air780EG MQTT连接状态响应格式: +CMQTTCONNECT: <client_index>,<state>
    // state: 0=已连接, 1=正在连接, 2=已断开, 3=连接失败
    if (response.indexOf("+CMQTTCONNECT: 0,0") >= 0) {
        return true;  // 已连接
    }
    
    // 也检查是否有连接成功的URC消息
    if (response.indexOf("+CMQTTCONNECT: 0") >= 0) {
        // 解析状态码
        int stateStart = response.indexOf("+CMQTTCONNECT: 0,") + 17;
        if (stateStart > 17 && stateStart < response.length()) {
            char stateChar = response.charAt(stateStart);
            debugPrint("Air780EG MQTT: 连接状态码: " + String(stateChar));
            return (stateChar == '0');  // 0表示已连接
        }
    }
    
    return false;
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
