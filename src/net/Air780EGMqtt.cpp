/*
 * Air780EG MQTT 管理器实现
 * 使用正确的官方AT指令
 */

#include "Air780EGMqtt.h"
#include "../utils/DebugUtils.h"
#include "../config.h"

// 静态实例指针，用于静态回调函数
static Air780EGMqtt* g_air780egMqttInstance = nullptr;

// 静态消息处理函数，用于注册到 Air780EGModem
static void staticMQTTMessageHandler(String topic, String payload) {
    if (g_air780egMqttInstance) {
        g_air780egMqttInstance->handleMQTTMessage(topic, payload);
    }
}

Air780EGMqtt::Air780EGMqtt(Air780EGModem& modem) 
    : _modem(modem), _debug(true), _connected(false), 
      _port(1883), _messageCallback(nullptr) {
    // 设置静态实例指针
    g_air780egMqttInstance = this;
}

bool Air780EGMqtt::begin() {
    debugPrint("Air780EG MQTT: 初始化");
    
    // 在 Air780EGModem 中注册静态消息处理器
    _modem.setMQTTMessageHandler(staticMQTTMessageHandler);
    debugPrint("Air780EG MQTT: 已在 Air780EGModem 中注册消息处理器");
    
    if (!_modem.isNetworkReady()) {
        debugPrint("Air780EG MQTT: 网络未就绪，但允许初始化，等待网络连接后再设置MQTT服务");
        return true; // 允许初始化，稍后在connect时再设置MQTT服务
    }
    
    return setupMQTT();
}

bool Air780EGMqtt::setupMQTT() {
    debugPrint("Air780EG MQTT: 开始设置MQTT服务");
    
    // 1. 检查网络注册状态
    String networkStatus = _modem.sendATWithResponse("AT+CREG?", 3000);
    debugPrint("Air780EG MQTT: 网络注册状态: " + networkStatus);
    
    // 检查网络是否已注册 (状态1或5表示已注册)
    bool networkRegistered = (networkStatus.indexOf(",1") >= 0 || networkStatus.indexOf(",5") >= 0);
    if (!networkRegistered) {
        debugPrint("Air780EG MQTT: ⚠️ 网络未注册，当前状态不适合MQTT连接");
        // 不立即返回false，让上层决定是否重试
    }
    
    // 2. 检查GPRS附着状态
    String gprsStatus = _modem.sendATWithResponse("AT+CGATT?", 3000);
    debugPrint("Air780EG MQTT: GPRS附着状态: " + gprsStatus);
    
    // 如果GPRS未附着，记录警告但不阻止连接尝试
    if (gprsStatus.indexOf("+CGATT: 0") >= 0) {
        debugPrint("Air780EG MQTT: ⚠️ GPRS未附着，可能影响MQTT连接");
    }
    
    // 3. 检查信号强度
    String signalStatus = _modem.sendATWithResponse("AT+CSQ", 3000);
    debugPrint("Air780EG MQTT: 信号强度: " + signalStatus);
    
    // 4. 检查MQTT连接状态
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
            
            // 6. 设置消息上报模式为直接上报 AT+MQTTMSGSET=0 后面走URC处理
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
    MQTT_DEBUG("连接状态查询: %s", response.c_str());
    
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
 * 格式：AT+MPUB="topic",qos,retain,"hex_payload"
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

    // 将JSON字符串转换为HEX编码
    String hexPayload = "";
    for (int i = 0; i < payload.length(); i++) {
        char c = payload.charAt(i);
        // 转换为两位十六进制
        if (c < 16) hexPayload += "0";
        hexPayload += String((unsigned char)c, HEX);
    }
    
    // 构建HEX模式的发布命令
    String cmd = "AT+MPUB=\"" + topic + "\"," + String(qos) + ",0,\"" + hexPayload + "\"";
    
    debugPrint("Air780EG MQTT: HEX载荷长度: " + String(hexPayload.length()));
    debugPrint("Air780EG MQTT: 原始消息: " + payload.substring(0, 30) + "...");
    debugPrint("Air780EG MQTT: HEX编码: " + hexPayload.substring(0, 60) + "...");
    debugPrint("Air780EG MQTT: 发布命令: " + cmd.substring(0, 120) + "...");

    // 尝试发布
    if (_modem.sendAT(cmd, "OK", 15000)) {
        debugPrint("Air780EG MQTT: ✅ HEX模式消息发布成功");
        return true;
    }

    debugPrint("Air780EG MQTT: ❌ HEX模式发布失败.");
    return false;
}

/**
 * 订阅MQTT主题
 * 按照官方文档格式：
 * AT+MSUB="topic",qos
 */
bool Air780EGMqtt::subscribe(const String& topic, int qos) {
    debugPrint("=== Air780EG MQTT 订阅开始 ===");
    debugPrint("Air780EG MQTT: 订阅请求 - 主题: " + topic);
    debugPrint("Air780EG MQTT: 订阅请求 - QoS: " + String(qos));
    
    if (!_connected) {
        debugPrint("Air780EG MQTT: 订阅失败 - 未连接到MQTT服务器");
        debugPrint("=== Air780EG MQTT 订阅结束 (失败) ===");
        return false;
    }

    debugPrint("Air780EG MQTT: MQTT连接状态正常，继续订阅");
    debugPrint("Air780EG MQTT: 回调函数状态: " + String(_messageCallback ? "已设置" : "未设置"));

    // 按照文档格式：AT+MSUB="topic",qos
    String cmd = "AT+MSUB=\"" + topic + "\"," + String(qos);
    debugPrint("Air780EG MQTT: 准备发送订阅命令: " + cmd);

    // 使用更详细的响应检查
    debugPrint("Air780EG MQTT: 发送AT命令，等待响应...");
    String response = _modem.sendATWithResponse(cmd, 8000);
    debugPrint("Air780EG MQTT: AT命令响应: " + response);
    debugPrint("Air780EG MQTT: 响应长度: " + String(response.length()));
    
    if (response.indexOf("OK") >= 0) {
        debugPrint("Air780EG MQTT: ✅ 订阅成功 - " + topic);
        debugPrint("=== Air780EG MQTT 订阅结束 (成功) ===");
        return true;
    } else if (response.indexOf("ERROR") >= 0) {
        debugPrint("Air780EG MQTT: ❌ 订阅失败 - ERROR响应");
        debugPrint("=== Air780EG MQTT 订阅结束 (ERROR) ===");
        return false;
    } else {
        debugPrint("Air780EG MQTT: ⚠️ 订阅超时或未知响应");
        debugPrint("=== Air780EG MQTT 订阅结束 (超时) ===");
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

/**
 * Air780EG MQTT客户端主循环
 * 
 * 优化说明：
 * - 使用直接上报模式，MQTT消息通过URC (+MSUB:) 自动上报到串口
 * - 移除了不必要的AT+MQTTMSGGET轮询，提高性能
 * - 实际的消息处理由Air780EGModem的handleURC方法完成
 * - loop函数主要用于维护连接状态和减少日志输出
 */
void Air780EGMqtt::loop() {
    // 在直接上报模式下，MQTT消息会通过URC (+MSUB:) 自动上报到串口
    // 这些消息由Air780EGModem的handleURC方法处理，无需在此轮询
    
    if (!_connected) {
        // 每30秒输出一次未连接状态，避免日志过多
        static unsigned long lastDisconnectedLog = 0;
        unsigned long now = millis();
        if (now - lastDisconnectedLog > 30000) {
            debugPrint("Air780EG MQTT: 未连接状态，跳过处理");
            lastDisconnectedLog = now;
        }
        return;
    }
}

void Air780EGMqtt::setCallback(void (*callback)(String topic, String payload)) {
    _messageCallback = callback;
}

void Air780EGMqtt::handleMQTTMessage(String topic, String payload) {
    debugPrint("Air780EG MQTT: 收到消息 - 主题: " + topic + ", 负载: " + payload);
    
    if (_messageCallback) {
        debugPrint("Air780EG MQTT: 调用应用层回调函数");
        _messageCallback(topic, payload);
    } else {
        debugPrint("Air780EG MQTT: 警告 - 应用层回调未设置，消息被忽略");
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
