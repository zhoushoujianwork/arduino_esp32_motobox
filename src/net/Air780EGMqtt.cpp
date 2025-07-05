/*
 * Air780EG MQTT 管理器实现
 * 使用正确的官方AT指令
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
    
    // 检查MQTT连接状态 - 使用正确的AT指令
    String response = _modem.sendATWithResponse("AT+MQTTSTATU", 3000);
    debugPrint("Air780EG MQTT: MQTT状态查询: " + response);
    
    // 如果返回ERROR，说明可能还未建立MQTT连接，这是正常的
    if (response.indexOf("ERROR") >= 0) {
        debugPrint("Air780EG MQTT: MQTT状态查询返回ERROR，这是正常的，表示还未建立MQTT连接");
        // 这不是错误，继续执行
    }
    
    debugPrint("Air780EG MQTT: MQTT功能检查完成");
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
    debugPrint("Air780EG MQTT: 客户端ID: " + _clientId);
    debugPrint("Air780EG MQTT: 用户名: " + (username.length() > 0 ? username : "无"));
    String passInfo = "Air780EG MQTT: 密码: ";
    passInfo += (password.length() > 0 ? "***已设置***" : "无");
    debugPrint(passInfo);
    
    if (!_modem.isNetworkReady()) {
        debugPrint("Air780EG MQTT: 网络未就绪");
        return false;
    }
    
    // 确保MQTT服务已启动
    if (!setupMQTT()) {
        debugPrint("Air780EG MQTT: MQTT服务设置失败");
        return false;
    }
    
    // 按照文档顺序执行MQTT连接
    // 1. 先关闭可能存在的连接
    debugPrint("Air780EG MQTT: 关闭现有连接...");
    String closeResp = _modem.sendATWithResponse("AT+MIPCLOSE", 1000);
    debugPrint("Air780EG MQTT: 关闭连接响应: " + closeResp);
    delay(500);
    
    // 2. 设置MQTT配置参数 (AT+MCONFIG)
    String configCmd;
    if (username.length() > 0 && password.length() > 0) {
        // 有用户名和密码
        configCmd = "AT+MCONFIG=\"" + _clientId + "\",\"" + username + "\",\"" + password + "\"";
        debugPrint("Air780EG MQTT: 使用认证连接");
    } else {
        // 无用户名密码，只设置客户端ID
        configCmd = "AT+MCONFIG=\"" + _clientId + "\"";
        debugPrint("Air780EG MQTT: 使用匿名连接");
    }
    
    debugPrint("Air780EG MQTT: 设置MQTT配置: " + configCmd);
    String configResp = _modem.sendATWithResponse(configCmd, 5000);
    debugPrint("Air780EG MQTT: 配置响应: " + configResp);
    
    if (configResp.indexOf("OK") < 0) {
        debugPrint("Air780EG MQTT: MQTT配置失败");
        return false;
    }
    debugPrint("Air780EG MQTT: MQTT配置成功");
    
    // 3. 建立TCP连接 (AT+MIPSTART)
    String connectCmd = "AT+MIPSTART=\"" + server + "\"," + String(port);
    debugPrint("Air780EG MQTT: 建立TCP连接: " + connectCmd);
    
    String tcpResp = _modem.sendATWithResponse(connectCmd, 15000);
    debugPrint("Air780EG MQTT: TCP连接响应: " + tcpResp);
    
    if (tcpResp.indexOf("OK") < 0) {
        debugPrint("Air780EG MQTT: TCP连接失败");
        return false;
    }
    debugPrint("Air780EG MQTT: TCP连接成功");
    
    // 等待TCP连接稳定
    delay(2000);
    
    // 4. 建立MQTT连接 (AT+MCONNECT=1,60) 客户端向服务器请求会话连接
    debugPrint("Air780EG MQTT: 建立MQTT连接...");
    
    // 增加重试机制
    int maxRetries = 3;
    bool connectSuccess = false;
    
    for (int retry = 0; retry < maxRetries && !connectSuccess; retry++) {
        if (retry > 0) {
            debugPrint("Air780EG MQTT: 重试连接 (" + String(retry + 1) + "/" + String(maxRetries) + ")");
            delay(5000);  // 重试前等待5秒
        }
        
        String mqttResp = _modem.sendATWithResponse("AT+MCONNECT=1,60", 15000);  // 增加超时时间
        debugPrint("Air780EG MQTT: MQTT连接响应: " + mqttResp);
        
        if (mqttResp.indexOf("CME ERROR: 3") >= 0) {
            debugPrint("Air780EG MQTT: MQTT连接被拒绝 (CME ERROR: 3)");
            debugPrint("Air780EG MQTT: 可能原因:");
            debugPrint("  1. 服务器要求认证但未提供用户名密码");
            debugPrint("  2. 用户名密码错误");
            debugPrint("  3. 客户端ID冲突");
            debugPrint("  4. 服务器配置问题");
            break;  // 认证错误不需要重试
        } else if (mqttResp.indexOf("ERROR") >= 0) {
            debugPrint("Air780EG MQTT: MQTT连接出现错误，将重试");
            continue;  // 其他错误可以重试
        } else if (mqttResp.indexOf("OK") >= 0) {
            connectSuccess = true;
            debugPrint("Air780EG MQTT: MQTT连接成功");
            
            // 5. 设置MQTT消息编码格式为HEX模式 (关键修复!)
            debugPrint("Air780EG MQTT: 设置消息编码格式为HEX模式...");
            String modeResp = _modem.sendATWithResponse("AT+MQTTMODE=1", 3000);
            debugPrint("Air780EG MQTT: 模式设置响应: " + modeResp);
            
            if (modeResp.indexOf("OK") >= 0) {
                debugPrint("Air780EG MQTT: ✅ HEX模式设置成功");
            } else {
                debugPrint("Air780EG MQTT: ⚠️ HEX模式设置失败，可能影响消息发布");
            }
            
            // 6. 可选：设置消息上报模式为直接上报
            debugPrint("Air780EG MQTT: 设置消息上报模式...");
            String msgSetResp = _modem.sendATWithResponse("AT+MQTTMSGSET=0", 3000);
            debugPrint("Air780EG MQTT: 消息模式设置响应: " + msgSetResp);
            
            break;
        } else if (mqttResp.length() == 0) {
            debugPrint("Air780EG MQTT: 连接超时，将重试");
            continue;  // 超时重试
        } else {
            debugPrint("Air780EG MQTT: 未知响应，将重试");
            continue;
        }
    }
    
    if (!connectSuccess) {
        debugPrint("Air780EG MQTT: MQTT连接失败，已达到最大重试次数");
        return false;
    }
    
    // 等待连接稳定
    delay(2000);
    
    // 5. 验证连接状态
    if (checkConnection()) {
        _connected = true;
        debugPrint("Air780EG MQTT: 连接验证成功！");
        return true;
    }
    
    debugPrint("Air780EG MQTT: 连接验证失败");
    return false;
}

bool Air780EGMqtt::disconnect() {
    debugPrint("Air780EG MQTT: 断开连接");
    
    bool success = true;
    
    // 断开MQTT连接 - 使用正确的AT指令
    if (!_modem.sendAT("AT+MDISCONNECT", "OK", 5000)) {
        debugPrint("Air780EG MQTT: 断开MQTT连接失败");
        success = false;
    }
    
    // 关闭TCP连接
    if (!_modem.sendAT("AT+MIPCLOSE", "OK", 3000)) {
        debugPrint("Air780EG MQTT: 关闭TCP连接失败");
        success = false;
    }
    
    _connected = false;
    
    if (success) {
        debugPrint("Air780EG MQTT: 断开连接成功");
    }
    
    return success;
}

bool Air780EGMqtt::isConnected() {
    static unsigned long lastCheck = 0;
    static bool lastResult = false;
    
    // 避免频繁检查，每5秒检查一次
    if (millis() - lastCheck > 5000) {
        lastCheck = millis();
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
    String response = _modem.sendATWithResponse("AT+MQTTSTATU", 3000);
    debugPrint("Air780EG MQTT: 连接状态查询: " + response);
    
    // 如果返回OK且没有ERROR，说明MQTT功能正常
    // 具体的连接状态需要根据返回内容判断
    if (response.indexOf("OK") >= 0 && response.indexOf("ERROR") < 0) {
        return true;
    }
    
    return false;
}

/**
 * 发布MQTT消息
 * 使用HEX编码模式：AT+MQTTMODE=1
 * 格式：AT+MPUB="topic",qos,retain,length,"hex_payload"
 */
bool Air780EGMqtt::publish(const String& topic, const String& payload, int qos) {
    if (!_connected) {
        debugPrint("Air780EG MQTT: 未连接，无法发布消息");
        return false;
    }

    debugPrint("Air780EG MQTT: 发布到 " + topic + " -> " + payload.substring(0, 50) + "...");
    debugPrint("Air780EG MQTT: 消息长度: " + String(payload.length()) + " 字节");

    // 检查消息长度限制
    if (payload.length() > 1024) {
        debugPrint("Air780EG MQTT: ⚠️ 消息过长 (" + String(payload.length()) + " > 1024)，可能发布失败");
    }

    // 设置为HEX模式
    debugPrint("Air780EG MQTT: 设置HEX编码模式...");
    if (!_modem.sendAT("AT+MQTTMODE=1", "OK", 3000)) {
        debugPrint("Air780EG MQTT: ⚠️ HEX模式设置失败");
    }

    // 将JSON字符串转换为HEX编码
    String hexPayload = "";
    for (int i = 0; i < payload.length(); i++) {
        char c = payload.charAt(i);
        // 转换为两位十六进制
        if (c < 16) hexPayload += "0";
        hexPayload += String((unsigned char)c, HEX);
    }
    
    // 构建HEX模式的发布命令
    // 格式：AT+MPUB="topic",qos,retain,length,"hex_payload"
    String cmd = "AT+MPUB=\"" + topic + "\"," + String(qos) + ",0," + String(payload.length()) + ",\"" + hexPayload + "\"";
    
    debugPrint("Air780EG MQTT: HEX载荷长度: " + String(hexPayload.length()));
    debugPrint("Air780EG MQTT: 原始消息: " + payload.substring(0, 30) + "...");
    debugPrint("Air780EG MQTT: HEX编码: " + hexPayload.substring(0, 60) + "...");
    debugPrint("Air780EG MQTT: 发布命令: " + cmd.substring(0, 120) + "...");

    // 尝试发布
    if (_modem.sendAT(cmd, "OK", 15000)) {
        debugPrint("Air780EG MQTT: ✅ HEX模式消息发布成功");
        return true;
    }

    debugPrint("Air780EG MQTT: ❌ HEX模式首次发布失败，开始重试...");

    // 重试机制
    int maxRetries = 3;
    for (int retry = 0; retry < maxRetries; retry++) {
        debugPrint("Air780EG MQTT: HEX模式重试发布 (" + String(retry + 1) + "/" + String(maxRetries) + ")");
        delay(2000); // 重试前等待2秒

        // 检查连接状态
        if (!checkConnection()) {
            debugPrint("Air780EG MQTT: 连接已断开，停止重试");
            _connected = false;
            break;
        }

        // 重新确保HEX模式
        _modem.sendAT("AT+MQTTMODE=1", "OK", 2000);

        // 重试发布
        if (_modem.sendAT(cmd, "OK", 15000)) {
            debugPrint("Air780EG MQTT: ✅ HEX模式重试成功 - 消息发布成功");
            return true;
        } else {
            debugPrint("Air780EG MQTT: 消息发布失败，尝试: " + String(retry + 1));

            // 检查连接状态
            if (!checkConnection()) {
                debugPrint("Air780EG MQTT: 连接已断开，停止重试");
                _connected = false;
                break;
            }
        }
    }

    debugPrint("Air780EG MQTT: 消息发布失败，已达到最大重试次数");
    return false;
}

/**
 * 订阅MQTT主题
 * 按照官方文档格式：
 * AT+MSUB="topic",qos
 */
bool Air780EGMqtt::subscribe(const String& topic, int qos) {
    if (!_connected) {
        debugPrint("Air780EG MQTT: 未连接，无法订阅");
        return false;
    }

    debugPrint("Air780EG MQTT: 订阅主题 " + topic);

    // 按照文档格式：AT+MSUB="topic",qos
    String cmd = "AT+MSUB=\"" + topic + "\"," + String(qos);

    debugPrint("Air780EG MQTT: 订阅命令: " + cmd);

    if (_modem.sendAT(cmd, "OK", 5000)) {
        debugPrint("Air780EG MQTT: 订阅成功");
        return true;
    } else {
        debugPrint("Air780EG MQTT: 订阅失败");
        return false;
    }
}

bool Air780EGMqtt::unsubscribe(const String& topic) {
    if (!_connected) {
        return false;
    }
    
    debugPrint("Air780EG MQTT: 取消订阅 " + topic);
    
    // 使用正确的AT指令格式：AT+MUNSUB="topic"
    String cmd = "AT+MUNSUB=\"" + topic + "\"";
    
    debugPrint("Air780EG MQTT: 取消订阅命令: " + cmd);
    
    if (_modem.sendAT(cmd, "OK", 5000)) {
        debugPrint("Air780EG MQTT: 取消订阅成功");
        return true;
    } else {
        debugPrint("Air780EG MQTT: 取消订阅失败");
        return false;
    }
}

void Air780EGMqtt::loop() {
    if (!_connected) return;
    
    // 限制消息检查频率，避免过于频繁的AT命令
    static unsigned long lastMsgCheck = 0;
    unsigned long now = millis();
    
    // 每5秒检查一次消息，避免过于频繁
    if (now - lastMsgCheck < 5000) {
        return;
    }
    lastMsgCheck = now;
    
    // 检查是否有新消息 - 使用正确的AT指令
    try {
        String response = _modem.sendATWithResponse("AT+MQTTMSGGET", 2000);
        if (response.length() > 0 && response.indexOf("+MQTTMSGGET:") >= 0) {
            handleIncomingMessage(response);
        }
    } catch (...) {
        debugPrint("Air780EG MQTT: 消息检查异常");
    }
}

void Air780EGMqtt::setCallback(void (*callback)(String topic, String payload)) {
    _messageCallback = callback;
}

void Air780EGMqtt::handleIncomingMessage(const String& response) {
    // 解析收到的MQTT消息
    // 格式可能是: +MQTTMSGGET: "topic","payload"
    debugPrint("Air780EG MQTT: 收到消息: " + response);
    
    // 安全检查：确保response不为空且包含有效数据
    if (response.length() == 0) {
        debugPrint("Air780EG MQTT: 收到空消息，忽略");
        return;
    }
    
    // 检查消息格式
    if (response.indexOf("+MQTTMSGGET:") < 0) {
        debugPrint("Air780EG MQTT: 消息格式不正确，忽略");
        return;
    }
    
    // 安全检查回调函数
    if (_messageCallback == nullptr) {
        debugPrint("Air780EG MQTT: 消息回调未设置，忽略消息");
        return;
    }
    
    try {
        // 简单的消息处理，避免复杂的字符串解析
        String topic = "air780eg/message";
        String payload = response.substring(response.indexOf(":") + 1);
        payload.trim();
        
        // 确保payload不为空
        if (payload.length() > 0) {
            _messageCallback(topic, payload);
        }
    } catch (...) {
        debugPrint("Air780EG MQTT: 消息处理异常，忽略");
    }
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
    
    // 测试各个MQTT AT指令是否被支持
    struct MQTTCommand {
        String command;
        String description;
    };
    
    MQTTCommand commands[] = {
        {"AT+MQTTSTATU", "查询MQTT连接状态"},
        {"AT+MCONFIG", "设置MQTT相关参数"},
        {"AT+MIPSTART", "建立TCP连接"},
        {"AT+MCONNECT", "客户端向服务器请求会话连接"},
        {"AT+MPUB", "发布消息"},
        {"AT+MSUB", "订阅主题"},
        {"AT+MUNSUB", "取消订阅主题"},
        {"AT+MQTTMSGGET", "打印收到的所有的订阅消息"},
        {"AT+MIPCLOSE", "关闭TCP连接"},
        {"AT+MDISCONNECT", "关闭MQTT连接"}
    };
    
    int supportedCount = 0;
    int totalCount = sizeof(commands) / sizeof(commands[0]);
    
    for (int i = 0; i < totalCount; i++) {
        debugPrint("测试指令: " + commands[i].command + " - " + commands[i].description);
        
        String response = _modem.sendATWithResponse(commands[i].command + "?", 3000);
        
        if (response.indexOf("ERROR") < 0) {
            debugPrint("✅ " + commands[i].command + " - 支持");
            supportedCount++;
        } else {
            debugPrint("❌ " + commands[i].command + " - 不支持");
        }
        
        delay(500); // 避免AT指令发送过快
    }
    
    debugPrint("MQTT功能测试完成: " + String(supportedCount) + "/" + String(totalCount) + " 指令支持");
    
    // 如果大部分指令都支持，认为MQTT功能可用
    bool isSupported = (supportedCount >= totalCount / 2);
    
    if (isSupported) {
        debugPrint("✅ Air780EG MQTT功能测试通过");
    } else {
        debugPrint("❌ Air780EG MQTT功能测试失败 - 固件可能不支持MQTT");
    }
    
    return isSupported;
}
