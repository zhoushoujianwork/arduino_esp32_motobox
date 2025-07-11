#ifndef MQTT_TOPICS_H
#define MQTT_TOPICS_H

#include <Arduino.h>

/**
 * MQTT主题管理类 - 应用层实现
 * 基于您已定义的主题结构进行扩展和规范化
 */
class MQTTTopics {
private:
    String device_id;
    String base_topic;           // vehicle/v1/{device_id}
    String telemetry_topic;      // vehicle/v1/{device_id}/telemetry/
    String status_topic;         // vehicle/v1/{device_id}/status
    String control_topic;        // vehicle/v1/{device_id}/ctrl/
    String config_topic;         // vehicle/v1/{device_id}/config/
    String event_topic;          // vehicle/v1/{device_id}/event/
    // GPS 定位主题
    String gps_topic;            // vehicle/v1/{device_id}/telemetry/location
    // IMU 主题
    String imu_topic;            // vehicle/v1/{device_id}/telemetry/motion
    // 系统状态主题
    String system_topic;         // vehicle/v1/{device_id}/system/
    // 设备信息主题
    String device_info_topic;    // vehicle/v1/{device_id}/telemetry/device

public:
    MQTTTopics(const String& deviceId) : device_id(deviceId) {
        base_topic = "vehicle/v1/" + device_id;
        telemetry_topic = base_topic + "/telemetry/";
        status_topic = base_topic + "/status";
        control_topic = base_topic + "/ctrl/";
        config_topic = base_topic + "/config/";
        event_topic = base_topic + "/event/";
        gps_topic = telemetry_topic + "location";
        imu_topic = telemetry_topic + "motion";
        system_topic = base_topic + "/system/";
        device_info_topic = telemetry_topic + "device";
    }

    // ==================== 基础主题 ====================
    String getBaseTopic() const { return base_topic; }
    String getTelemetryTopic() const { return telemetry_topic; }
    String getStatusTopic() const { return status_topic; }
    String getControlTopic() const { return control_topic; }
    String getConfigTopic() const { return config_topic; }
    String getEventTopic() const { return event_topic; }
    String getGPSTopic() const { return gps_topic; }
    String getIMUTopic() const { return imu_topic; }
    String getSystemTopic() const { return system_topic; }
    String getDeviceInfoTopic() const { return device_info_topic; }

    
    // ==================== 事件主题 (event) ====================
    String getEventAlarmTopic() const { return event_topic + "alarm"; }
    String getEventWarningTopic() const { return event_topic + "warning"; }
    String getEventInfoTopic() const { return event_topic + "info"; }
    String getEventSecurityTopic() const { return event_topic + "security"; }
    String getEventMaintenanceTopic() const { return event_topic + "maintenance"; }

    // ==================== OTA升级主题 ====================
    String getOTACommandTopic() const { return base_topic + "/ota/command"; }
    String getOTAStatusTopic() const { return base_topic + "/ota/status"; }
    String getOTAProgressTopic() const { return base_topic + "/ota/progress"; }

    // ==================== 调试主题 ====================
    String getDebugLogTopic() const { return base_topic + "/debug/log"; }
    String getDebugMetricsTopic() const { return base_topic + "/debug/metrics"; }

    // ==================== 订阅主题列表 ====================
    // 获取需要订阅的所有控制主题
    void getSubscriptionTopics(String topics[], int& count) const {
        count = 0;
        topics[count++] = control_topic + "#";        // 所有控制命令
        topics[count++] = config_topic + "#";         // 所有配置命令
        topics[count++] = getOTACommandTopic();       // OTA命令
    }

    // ==================== 主题验证和解析 ====================
    bool isControlTopic(const String& topic) const {
        return topic.startsWith(control_topic);
    }
    
    bool isConfigTopic(const String& topic) const {
        return topic.startsWith(config_topic);
    }
    
    bool isTelemetryTopic(const String& topic) const {
        return topic.startsWith(telemetry_topic);
    }
    
    bool isOTATopic(const String& topic) const {
        return topic.indexOf("/ota/") >= 0;
    }

    // 从主题中提取命令类型
    String extractCommand(const String& topic) const {
        if (isControlTopic(topic)) {
            return topic.substring(control_topic.length());
        } else if (isConfigTopic(topic)) {
            return topic.substring(config_topic.length());
        }
        return "";
    }

    // ==================== 调试功能 ====================
    void printTopicStructure() const {
        Serial.println("=== MQTT Topic Structure ===");
        Serial.println("Device ID: " + device_id);
        Serial.println("Base Topic: " + base_topic);
        Serial.println();
        
        Serial.println("Telemetry Topics:");
        Serial.println("  Device Info: " + getDeviceInfoTopic());
        Serial.println("  GPS Location: " + getGPSTopic());
        Serial.println("  IMU Motion: " + getIMUTopic());
        Serial.println();
        
        Serial.println("Event Topics:");
        Serial.println("  Alarm: " + getEventAlarmTopic());
        Serial.println("  Warning: " + getEventWarningTopic());
        Serial.println();
        
        Serial.println("Subscription Topics:");
        String subs[10];
        int count;
        getSubscriptionTopics(subs, count);
        for (int i = 0; i < count; i++) {
            Serial.println("  " + subs[i]);
        }
        Serial.println("============================");
    }
};

extern MQTTTopics mqttTopics;

#endif // MQTT_TOPICS_H
