/*
 * Air780EG MQTT 管理器
 * 基于Air780EG模块的MQTT通信功能
 */

#ifndef AIR780EG_MQTT_H
#define AIR780EG_MQTT_H

#include <Arduino.h>
#include "Air780EGModem.h"

class Air780EGMqtt {
public:
    Air780EGMqtt(Air780EGModem& modem);
    
    // MQTT连接管理
    bool begin();
    bool connect(const String& server, int port, const String& clientId, 
                const String& username = "", const String& password = "");
    bool disconnect();
    bool isConnected();
    
    // 消息发布和订阅
    bool publish(const String& topic, const String& payload, int qos = 0);
    bool subscribe(const String& topic, int qos = 0);
    bool unsubscribe(const String& topic);
    
    // 消息处理
    void loop();
    void setCallback(void (*callback)(String topic, String payload));
    
    // 状态检查
    bool checkConnection();
    void setDebug(bool debug);
    bool testMQTTSupport();  // 测试MQTT功能支持
    
private:
    Air780EGModem& _modem;
    bool _debug;
    bool _connected;
    String _server;
    int _port;
    String _clientId;
    String _username;
    String _password;
    
    void (*_messageCallback)(String topic, String payload);
    
    void debugPrint(const String& msg);
    bool setupMQTT();
    void handleIncomingMessage(const String& data);
    void parseMSUBMessage(const String& data);  // 新增：解析MSUB消息
};

#endif // AIR780EG_MQTT_H
