/*
 * Air780EG 集成示例
 * 展示如何将Air780EG集成到现有的MotoBox系统中
 */

#include "Air780EGModem.h"
#include "Air780EGMqtt.h"

#ifdef ENABLE_AIR780EG

// 前置声明
void handleMQTTMessage(String topic, String payload);
void handleControlCommand(const String& command);
void handleConfigCommand(const String& config);
void recordGNSSData();
String createGPSJSON(const GNSSData& gnss);
void publishDeviceStatus();
void publishNetworkStatus();
void publishGNSSStatus();

// 全局实例
Air780EGMqtt* air780eg_mqtt = nullptr;

// 状态变量
bool air780eg_initialized = false;
unsigned long last_gnss_record = 0;
unsigned long last_status_publish = 0;

// MQTT配置 (应该从配置文件或EEPROM读取)
const char* AIR780EG_MQTT_SERVER = "your-mqtt-server.com";
const int AIR780EG_MQTT_PORT = 1883;
const char* AIR780EG_MQTT_CLIENT_ID = "motobox-air780eg";
const char* AIR780EG_MQTT_USERNAME = "";
const char* AIR780EG_MQTT_PASSWORD = "";

// 初始化Air780EG系统
bool initializeAir780EG() {
    Serial.println("初始化Air780EG系统...");
    
    // 设置调试模式
    air780eg_modem.setDebug(true);
    
    // 初始化模块
    if (!air780eg_modem.begin()) {
        Serial.println("Air780EG模块初始化失败");
        return false;
    }
    
    // 等待网络注册
    Serial.println("等待网络注册...");
    int retry = 0;
    while (!air780eg_modem.isNetworkReady() && retry < 30) {
        Serial.print(".");
        delay(2000);
        retry++;
    }
    
    if (!air780eg_modem.isNetworkReady()) {
        Serial.println("\n网络注册失败");
        return false;
    }
    
    Serial.println("\n网络注册成功");
    air780eg_modem.debugNetworkInfo();
    
    // 启用GNSS
    if (air780eg_modem.enableGNSS(true)) {
        Serial.println("GNSS启用成功");
        air780eg_modem.setGNSSUpdateRate(1);  // 1Hz更新频率
    } else {
        Serial.println("GNSS启用失败");
    }
    
    // 初始化MQTT
    air780eg_mqtt = new Air780EGMqtt(air780eg_modem);
    air780eg_mqtt->setDebug(true);
    
    if (air780eg_mqtt->begin()) {
        if (air780eg_mqtt->connect(AIR780EG_MQTT_SERVER, AIR780EG_MQTT_PORT, AIR780EG_MQTT_CLIENT_ID, AIR780EG_MQTT_USERNAME, AIR780EG_MQTT_PASSWORD)) {
            Serial.println("MQTT连接成功");
            
            // 订阅控制主题
            air780eg_mqtt->subscribe("motobox/control", 1);
            air780eg_mqtt->subscribe("motobox/config", 1);
            
            // 设置消息回调
            air780eg_mqtt->setCallback(handleMQTTMessage);
            
            air780eg_initialized = true;
            return true;
        } else {
            Serial.println("MQTT连接失败");
        }
    } else {
        Serial.println("MQTT初始化失败");
    }
    
    return false;
}

// 处理MQTT消息
void handleMQTTMessage(String topic, String payload) {
    Serial.println("收到MQTT消息: " + topic + " -> " + payload);
    
    if (topic == "motobox/control") {
        handleControlCommand(payload);
    }
    else if (topic == "motobox/config") {
        handleConfigCommand(payload);
    }
}

// 处理控制命令
void handleControlCommand(const String& command) {
    if (command == "status") {
        publishDeviceStatus();
    }
    else if (command == "reboot") {
        Serial.println("收到重启命令");
        ESP.restart();
    }
    else if (command == "gnss_info") {
        air780eg_modem.debugGNSSInfo();
        publishGNSSStatus();
    }
    else if (command == "network_info") {
        air780eg_modem.debugNetworkInfo();
        publishNetworkStatus();
    }
    // 可以添加更多控制命令
}

// 处理配置命令
void handleConfigCommand(const String& config) {
    // 解析JSON配置 (简化示例)
    if (config.indexOf("gnss_rate") >= 0) {
        // 提取GNSS更新频率配置
        int rate = 1; // 默认1Hz
        // 这里应该解析JSON获取实际值
        air780eg_modem.setGNSSUpdateRate(rate);
        Serial.println("GNSS更新频率设置为: " + String(rate) + "Hz");
    }
}

// Air780EG主循环处理
void processAir780EG() {
    if (!air780eg_initialized) return;
    
    unsigned long now = millis();
    
    // 处理MQTT消息
    if (air780eg_mqtt && air780eg_mqtt->isConnected()) {
        air780eg_mqtt->loop();
    }
    
    // 更新GNSS数据
    air780eg_modem.updateGNSSData();
    
    // 记录GPS数据到SD卡 (每5秒一次)
    if (now - last_gnss_record > 5000) {
        last_gnss_record = now;
        recordGNSSData();
    }
    
    // 发布状态数据 (每30秒一次)
    if (now - last_status_publish > 30000) {
        last_status_publish = now;
        publishDeviceStatus();
    }
    
    // 检查网络连接状态
    if (!air780eg_modem.isNetworkReady()) {
        Serial.println("网络连接丢失，尝试重连...");
        // 可以添加重连逻辑
    }
}

// 记录GNSS数据到SD卡
void recordGNSSData() {
    if (!air780eg_modem.isGNSSDataValid()) return;
    
    // 获取GNSS数据
    GNSSData gnss = air780eg_modem.getGNSSData();
    
    // 转换为兼容的GPS格式
    gps_data_t gps = air780eg_modem.getGPSData();
    
    // 使用现有的SD卡记录功能
    // 这里假设有一个全局的sdManager实例
    #ifdef ENABLE_SDCARD
    // sdManager.recordGPSData(gps);  // 使用现有的GPS记录功能
    #endif
    
    // 也可以发布到MQTT
    if (air780eg_mqtt && air780eg_mqtt->isConnected()) {
        String gpsJson = createGPSJSON(gnss);
        air780eg_mqtt->publish("motobox/gps", gpsJson, 0);
    }
}

// 创建GPS JSON数据
String createGPSJSON(const GNSSData& gnss) {
    String json = "{";
    json += "\"timestamp\":" + String(millis()) + ",";
    json += "\"valid\":" + String(gnss.valid ? "true" : "false") + ",";
    json += "\"latitude\":" + String(gnss.latitude, 6) + ",";
    json += "\"longitude\":" + String(gnss.longitude, 6) + ",";
    json += "\"altitude\":" + String(gnss.altitude, 2) + ",";
    json += "\"speed\":" + String(gnss.speed, 2) + ",";
    json += "\"course\":" + String(gnss.course, 2) + ",";
    json += "\"satellites\":" + String(gnss.satellites) + ",";
    json += "\"hdop\":" + String(gnss.hdop, 2) + ",";
    json += "\"timestamp_gnss\":\"" + gnss.timestamp + "\",";
    json += "\"fix_mode\":\"" + gnss.fix_mode + "\"";
    json += "}";
    return json;
}

// 发布设备状态
void publishDeviceStatus() {
    if (!air780eg_mqtt || !air780eg_mqtt->isConnected()) return;
    
    String status = "{";
    status += "\"timestamp\":" + String(millis()) + ",";
    status += "\"uptime\":" + String(millis() / 1000) + ",";
    status += "\"network\":{";
    status += "\"connected\":" + String(air780eg_modem.isNetworkReady() ? "true" : "false") + ",";
    status += "\"signal\":" + String(air780eg_modem.getCSQ()) + ",";
    status += "\"carrier\":\"" + air780eg_modem.getCarrierName() + "\",";
    status += "\"type\":\"" + air780eg_modem.getNetworkType() + "\",";
    status += "\"ip\":\"" + air780eg_modem.getLocalIP() + "\"";
    status += "},";
    
    // 添加GNSS状态
    status += "\"gnss\":{";
    status += "\"enabled\":" + String(air780eg_modem.isGNSSEnabled() ? "true" : "false") + ",";
    status += "\"valid\":" + String(air780eg_modem.isGNSSDataValid() ? "true" : "false");
    if (air780eg_modem.isGNSSDataValid()) {
        GNSSData gnss = air780eg_modem.getGNSSData();
        status += ",\"satellites\":" + String(gnss.satellites);
        status += ",\"hdop\":" + String(gnss.hdop, 2);
    }
    status += "},";
    
    // 添加系统状态
    status += "\"system\":{";
    status += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    status += "\"cpu_freq\":" + String(ESP.getCpuFreqMHz());
    status += "}";
    
    status += "}";
    
    air780eg_mqtt->publish("motobox/status", status, 1);
}

// 发布网络状态
void publishNetworkStatus() {
    if (!air780eg_mqtt || !air780eg_mqtt->isConnected()) return;
    
    String network = "{";
    network += "\"imei\":\"" + air780eg_modem.getIMEI() + "\",";
    network += "\"iccid\":\"" + air780eg_modem.getICCID() + "\",";
    network += "\"module\":\"" + air780eg_modem.getModuleName() + "\",";
    network += "\"carrier\":\"" + air780eg_modem.getCarrierName() + "\",";
    network += "\"network_type\":\"" + air780eg_modem.getNetworkType() + "\",";
    network += "\"signal_strength\":" + String(air780eg_modem.getCSQ()) + ",";
    network += "\"local_ip\":\"" + air780eg_modem.getLocalIP() + "\",";
    network += "\"connected\":" + String(air780eg_modem.isNetworkReady() ? "true" : "false");
    network += "}";
    
    air780eg_mqtt->publish("motobox/network", network, 1);
}

// 发布GNSS状态
void publishGNSSStatus() {
    if (!air780eg_mqtt || !air780eg_mqtt->isConnected()) return;
    
    String gnssStatus = "{";
    gnssStatus += "\"enabled\":" + String(air780eg_modem.isGNSSEnabled() ? "true" : "false") + ",";
    gnssStatus += "\"valid\":" + String(air780eg_modem.isGNSSDataValid() ? "true" : "false");
    
    if (air780eg_modem.isGNSSDataValid()) {
        GNSSData gnss = air780eg_modem.getGNSSData();
        gnssStatus += ",\"latitude\":" + String(gnss.latitude, 6);
        gnssStatus += ",\"longitude\":" + String(gnss.longitude, 6);
        gnssStatus += ",\"altitude\":" + String(gnss.altitude, 2);
        gnssStatus += ",\"speed\":" + String(gnss.speed, 2);
        gnssStatus += ",\"course\":" + String(gnss.course, 2);
        gnssStatus += ",\"satellites\":" + String(gnss.satellites);
        gnssStatus += ",\"hdop\":" + String(gnss.hdop, 2);
        gnssStatus += ",\"fix_mode\":\"" + gnss.fix_mode + "\"";
        gnssStatus += ",\"timestamp\":\"" + gnss.timestamp + "\"";
    }
    
    gnssStatus += "}";
    
    air780eg_mqtt->publish("motobox/gnss", gnssStatus, 1);
}

// 获取Air780EG状态 (供其他模块调用)
bool isAir780EGReady() {
    return air780eg_initialized && air780eg_modem.isNetworkReady();
}

// 获取当前位置 (供其他模块调用)
bool getCurrentLocation(float& lat, float& lon) {
    if (air780eg_modem.isGNSSDataValid()) {
        GNSSData gnss = air780eg_modem.getGNSSData();
        lat = gnss.latitude;
        lon = gnss.longitude;
        return true;
    }
    return false;
}

// 发送自定义MQTT消息 (供其他模块调用)
bool publishMQTTMessage(const String& topic, const String& payload, int qos) {
    if (air780eg_mqtt && air780eg_mqtt->isConnected()) {
        return air780eg_mqtt->publish(topic, payload, qos);
    }
    return false;
}

#endif // ENABLE_AIR780EG
