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
    
    // 检查网络注册状态
    String networkStatus = _modem.sendATWithResponse("AT+CREG?", 3000);
    debugPrint("Air780EG MQTT: 网络注册状态: " + networkStatus);
    
    // 检查GPRS附着状态
    String gprsStatus = _modem.sendATWithResponse("AT+CGATT?", 3000);
    debugPrint("Air780EG MQTT: GPRS附着状态: " + gprsStatus);
    
    // 先检查MQTT服务状态
    String response = _modem.sendATWithResponse("AT+CMQTTSTART?", 3000);
    debugPrint("Air780EG MQTT: 服务状态查询: " + response);
    
    // 如果返回ERROR，可能是固件不支持或需要先设置其他参数
    if (response.indexOf("ERROR") >= 0) {
        debugPrint("Air780EG MQTT: MQTT查询失败，可能固件不支持MQTT功能");
        
        // 尝试检查固件版本
        String version = _modem.sendATWithResponse("AT+CGMR", 3000);
        debugPrint("Air780EG MQTT: 固件版本: " + version);
        
        // 尝试检查模块型号
        String model = _modem.sendATWithResponse("AT+CGMM", 3000);
        debugPrint("Air780EG MQTT: 模块型号: " + model);
        
        return false;
    }
    
    // 如果服务已经启动，直接返回成功
    if (response.indexOf("+CMQTTSTART:") >= 0) {
        debugPrint("Air780EG MQTT: MQTT服务已启动");
        return true;
    }
    
    // 尝试停止可能存在的服务（忽略错误）
    _modem.sendAT("AT+CMQTTSTOP", "OK", 3000);
    delay(1000);
    
    // 启动MQTT服务
    debugPrint("Air780EG MQTT: 尝试启动MQTT服务...");
    if (_modem.sendAT("AT+CMQTTSTART", "OK", 10000)) {
        debugPrint("Air780EG MQTT: MQTT服务启动成功");
        
        // 再次查询状态确认
        String confirmResponse = _modem.sendATWithResponse("AT+CMQTTSTART?", 3000);
        debugPrint("Air780EG MQTT: 启动后状态确认: " + confirmResponse);
        
        return true;
    } else {
        debugPrint("Air780EG MQTT: MQTT服务启动失败");
        
        // 获取详细错误信息
        String errorResponse = _modem.sendATWithResponse("AT+CMEE=2", 1000);
        debugPrint("Air780EG MQTT: 启用详细错误信息: " + errorResponse);
        
        // 再次尝试启动并获取错误信息
        errorResponse = _modem.sendATWithResponse("AT+CMQTTSTART", 5000);
        debugPrint("Air780EG MQTT: 详细错误响应: " + errorResponse);
        
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
    
    // 根据官方文档，获取客户端索引的正确格式
    String acquireCmd = "AT+CMQTTACCQ=0,\"" + clientId + "\"";
    if (username.length() > 0) {
        acquireCmd += ",\"" + username + "\",\"" + password + "\"";
    }
    
    debugPrint("Air780EG MQTT: 获取客户端命令: " + acquireCmd);
    if (!_modem.sendAT(acquireCmd, "OK", 5000)) {
        debugPrint("Air780EG MQTT: 获取客户端失败");
        return false;
    }
    
    debugPrint("Air780EG MQTT: 客户端获取成功");
    
    // 根据官方文档，连接命令的正确格式
    String connectCmd = "AT+CMQTTCONNECT=0,\"tcp://" + server + ":" + String(port) + "\",60,1";
    
    debugPrint("Air780EG MQTT: 发送连接命令: " + connectCmd);
    
    if (_modem.sendAT(connectCmd, "OK", 15000)) {
        debugPrint("Air780EG MQTT: 连接命令发送成功，等待连接确认...");
        
        // 等待连接建立，检查URC消息
        for (int i = 0; i < 15; i++) {
            delay(1000);
            
            // 检查是否有连接成功的URC消息
            String urcResponse = _modem.sendATWithResponse("", 500);
            if (urcResponse.indexOf("+CMQTTCONNECT:") >= 0) {
                debugPrint("Air780EG MQTT: 收到连接URC: " + urcResponse);
                if (urcResponse.indexOf(",0") >= 0) { // 连接成功
                    _connected = true;
                    debugPrint("Air780EG MQTT: 连接成功！");
                    return true;
                }
            }
            
            // 也检查连接状态
            if (checkConnection()) {
                _connected = true;
                debugPrint("Air780EG MQTT: 连接成功！");
                return true;
            }
            debugPrint("Air780EG MQTT: 等待连接... (" + String(i+1) + "/15)");
        }
        
        debugPrint("Air780EG MQTT: 连接超时");
    } else {
        debugPrint("Air780EG MQTT: 连接命令发送失败");
        
        // 获取详细错误信息
        String errorResponse = _modem.sendATWithResponse("AT+CMQTTCONNECT=0,\"tcp://" + server + ":" + String(port) + "\",60,1", 5000);
        debugPrint("Air780EG MQTT: 连接错误详情: " + errorResponse);
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

bool Air780EGMqtt::testMQTTSupport() {
    debugPrint("Air780EG MQTT: 开始测试MQTT功能支持");
    
    // 1. 检查基本AT命令响应
    String basicResponse = _modem.sendATWithResponse("AT", 1000);
    debugPrint("Air780EG MQTT: 基本AT响应: " + basicResponse);
    
    // 2. 检查模块信息
    String model = _modem.sendATWithResponse("AT+CGMM", 3000);
    debugPrint("Air780EG MQTT: 模块型号: " + model);
    
    String version = _modem.sendATWithResponse("AT+CGMR", 3000);
    debugPrint("Air780EG MQTT: 固件版本: " + version);
    
    String imei = _modem.sendATWithResponse("AT+CGSN", 3000);
    debugPrint("Air780EG MQTT: IMEI: " + imei);
    
    // 3. 检查网络状态
    String networkReg = _modem.sendATWithResponse("AT+CREG?", 3000);
    debugPrint("Air780EG MQTT: 网络注册: " + networkReg);
    
    String gprsAttach = _modem.sendATWithResponse("AT+CGATT?", 3000);
    debugPrint("Air780EG MQTT: GPRS附着: " + gprsAttach);
    
    String signalQuality = _modem.sendATWithResponse("AT+CSQ", 3000);
    debugPrint("Air780EG MQTT: 信号质量: " + signalQuality);
    
    // 4. 启用详细错误报告
    _modem.sendAT("AT+CMEE=2", "OK", 1000);
    
    // 5. 测试MQTT命令支持
    debugPrint("Air780EG MQTT: 测试MQTT命令支持...");
    
    // 测试MQTT START查询
    String mqttStartQuery = _modem.sendATWithResponse("AT+CMQTTSTART?", 5000);
    debugPrint("Air780EG MQTT: MQTT START查询: " + mqttStartQuery);
    
    if (mqttStartQuery.indexOf("ERROR") >= 0) {
        debugPrint("Air780EG MQTT: ❌ MQTT功能不支持或固件版本过低");
        
        // 尝试其他可能的MQTT命令格式
        String altMqttTest = _modem.sendATWithResponse("AT+MQTTSTART?", 3000);
        debugPrint("Air780EG MQTT: 备用MQTT命令测试: " + altMqttTest);
        
        return false;
    }
    
    // 测试MQTT STOP（应该返回OK即使没有启动）
    String mqttStop = _modem.sendATWithResponse("AT+CMQTTSTOP", 3000);
    debugPrint("Air780EG MQTT: MQTT STOP测试: " + mqttStop);
    
    // 测试MQTT START
    String mqttStart = _modem.sendATWithResponse("AT+CMQTTSTART", 10000);
    debugPrint("Air780EG MQTT: MQTT START测试: " + mqttStart);
    
    if (mqttStart.indexOf("OK") >= 0) {
        debugPrint("Air780EG MQTT: ✅ MQTT功能支持正常");
        
        // 查询启动后状态
        String startedStatus = _modem.sendATWithResponse("AT+CMQTTSTART?", 3000);
        debugPrint("Air780EG MQTT: 启动后状态: " + startedStatus);
        
        return true;
    } else {
        debugPrint("Air780EG MQTT: ❌ MQTT启动失败: " + mqttStart);
        return false;
    }
}
