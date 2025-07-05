/*
 * Air780EG MQTT连接调试程序
 * 专门用于调试MQTT连接问题
 */

#include "Air780EGModem.h"

class Air780EGMqttDebugger {
private:
    Air780EGModem* _modem;
    
public:
    Air780EGMqttDebugger(Air780EGModem* modem) : _modem(modem) {}
    
    void debugMqttConnection() {
        Serial.println("=== Air780EG MQTT连接调试开始 ===");
        
        // 1. 检查网络状态
        checkNetworkStatus();
        
        // 2. 测试MQTT命令支持
        testMqttCommands();
        
        // 3. 逐步调试MQTT连接过程
        debugMqttConnectionProcess();
        
        Serial.println("=== Air780EG MQTT连接调试结束 ===");
    }
    
private:
    void checkNetworkStatus() {
        Serial.println("\n--- 网络状态检查 ---");
        
        // 检查网络注册
        String creg = _modem->sendATWithResponse("AT+CREG?", 3000);
        Serial.println("网络注册状态: " + creg);
        
        // 检查信号强度
        String csq = _modem->sendATWithResponse("AT+CSQ", 3000);
        Serial.println("信号强度: " + csq);
        
        // 检查GPRS附着
        String cgatt = _modem->sendATWithResponse("AT+CGATT?", 3000);
        Serial.println("GPRS附着状态: " + cgatt);
        
        // 检查PDP上下文
        String cgdcont = _modem->sendATWithResponse("AT+CGDCONT?", 3000);
        Serial.println("PDP上下文: " + cgdcont);
    }
    
    void testMqttCommands() {
        Serial.println("\n--- MQTT命令支持测试 ---");
        
        struct Command {
            String cmd;
            String desc;
        };
        
        Command commands[] = {
            {"AT+MQTTSTATU", "MQTT状态查询"},
            {"AT+MCONFIG", "MQTT配置"},
            {"AT+MIPSTART", "TCP连接"},
            {"AT+MCONNECT", "MQTT连接"},
            {"AT+MPUB", "消息发布"},
            {"AT+MSUB", "主题订阅"}
        };
        
        for (int i = 0; i < 6; i++) {
            Serial.print("测试 " + commands[i].cmd + " (" + commands[i].desc + "): ");
            
            String response = _modem->sendATWithResponse(commands[i].cmd + "?", 3000);
            
            if (response.indexOf("ERROR") >= 0) {
                Serial.println("❌ 不支持");
            } else {
                Serial.println("✅ 支持");
                Serial.println("  响应: " + response);
            }
            
            delay(500);
        }
    }
    
    void debugMqttConnectionProcess() {
        Serial.println("\n--- MQTT连接过程调试 ---");
        
        String server = "mq-hub.daboluo.cc";
        int port = 32571;
        String clientId = "test_client_" + String(millis());
        
        Serial.println("服务器: " + server + ":" + String(port));
        Serial.println("客户端ID: " + clientId);
        
        // 步骤1: 关闭现有连接
        Serial.println("\n1. 关闭现有连接...");
        String closeResp = _modem->sendATWithResponse("AT+MIPCLOSE", 3000);
        Serial.println("关闭响应: " + closeResp);
        delay(1000);
        
        // 步骤2: 设置MQTT配置
        Serial.println("\n2. 设置MQTT配置...");
        String configCmd = "AT+MCONFIG=\"" + clientId + "\"";
        Serial.println("配置命令: " + configCmd);
        
        String configResp = _modem->sendATWithResponse(configCmd, 5000);
        Serial.println("配置响应: " + configResp);
        
        if (configResp.indexOf("OK") < 0) {
            Serial.println("❌ MQTT配置失败，停止调试");
            return;
        }
        
        // 步骤3: 建立TCP连接
        Serial.println("\n3. 建立TCP连接...");
        String tcpCmd = "AT+MIPSTART=\"" + server + "\"," + String(port);
        Serial.println("TCP命令: " + tcpCmd);
        
        String tcpResp = _modem->sendATWithResponse(tcpCmd, 15000);
        Serial.println("TCP响应: " + tcpResp);
        
        if (tcpResp.indexOf("OK") < 0) {
            Serial.println("❌ TCP连接失败，停止调试");
            return;
        }
        
        // 等待CONNECT OK
        Serial.println("等待TCP连接确认...");
        delay(3000);
        
        // 读取可能的CONNECT OK消息
        delay(1000);  // 等待可能的连接状态消息
        
        // 步骤4: 建立MQTT连接
        Serial.println("\n4. 建立MQTT连接...");
        
        // 启用详细错误信息
        _modem->sendATWithResponse("AT+CMEE=2", 3000);
        
        String mqttResp = _modem->sendATWithResponse("AT+MCONNECT", 10000);
        Serial.println("MQTT连接响应: " + mqttResp);
        
        if (mqttResp.indexOf("CME ERROR") >= 0) {
            Serial.println("❌ MQTT连接失败，分析错误...");
            analyzeMqttError(mqttResp);
        } else if (mqttResp.indexOf("OK") >= 0) {
            Serial.println("✅ MQTT连接成功！");
            
            // 验证连接状态
            delay(2000);
            String statusResp = _modem->sendATWithResponse("AT+MQTTSTATU", 3000);
            Serial.println("连接状态验证: " + statusResp);
        }
        
        // 清理连接
        Serial.println("\n5. 清理连接...");
        _modem->sendATWithResponse("AT+MDISCONNECT", 3000);
        _modem->sendATWithResponse("AT+MIPCLOSE", 3000);
    }
    
    void analyzeMqttError(const String& errorResponse) {
        Serial.println("--- MQTT错误分析 ---");
        
        if (errorResponse.indexOf("CME ERROR: 3") >= 0) {
            Serial.println("错误代码: CME ERROR 3");
            Serial.println("可能原因:");
            Serial.println("  1. 操作不被允许 - 可能是MQTT配置参数有问题");
            Serial.println("  2. 服务器拒绝连接 - 检查服务器是否正常");
            Serial.println("  3. 认证失败 - 检查用户名密码");
            Serial.println("  4. 客户端ID冲突 - 尝试使用不同的客户端ID");
            
            // 尝试一些诊断命令
            Serial.println("\n诊断信息:");
            
            String mqttStatus = _modem->sendATWithResponse("AT+MQTTSTATU", 3000);
            Serial.println("MQTT状态: " + mqttStatus);
            
            // 尝试不同的配置
            Serial.println("\n尝试简化配置...");
            String simpleClientId = "simple_test";
            String simpleConfig = "AT+MCONFIG=\"" + simpleClientId + "\"";
            String simpleResp = _modem->sendATWithResponse(simpleConfig, 3000);
            Serial.println("简化配置响应: " + simpleResp);
            
        } else {
            Serial.println("未知MQTT错误: " + errorResponse);
        }
    }
};

// 全局调试函数
void debugAir780EGMqtt(Air780EGModem* modem) {
    Air780EGMqttDebugger debugger(modem);
    debugger.debugMqttConnection();
}
