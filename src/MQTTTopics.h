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

    // ==================== 遥测数据主题 (telemetry) ====================
    // 设备基础信息
    String getDeviceInfoTopic() const { return telemetry_topic + "device"; }
    String getDeviceHealthTopic() const { return telemetry_topic + "health"; }
    
    // 位置和运动数据
    String getGPSTopic() const { return telemetry_topic + "location"; }
    String getIMUTopic() const { return telemetry_topic + "motion"; }
    String getSpeedTopic() const { return telemetry_topic + "speed"; }
    
    // 传感器数据
    String getTemperatureTopic() const { return telemetry_topic + "temperature"; }
    String getVoltageTopic() const { return telemetry_topic + "voltage"; }
    String getVibrationTopic() const { return telemetry_topic + "vibration"; }
    String getFuelTopic() const { return telemetry_topic + "fuel"; }
    String getEngineTopic() const { return telemetry_topic + "engine"; }
    
    // 网络和信号
    String getNetworkTopic() const { return telemetry_topic + "network"; }
    String getSignalTopic() const { return telemetry_topic + "signal"; }
    
    // 电源管理
    String getBatteryTopic() const { return telemetry_topic + "battery"; }
    String getPowerTopic() const { return telemetry_topic + "power"; }

    // ==================== 状态主题 (status) ====================
    String getDeviceStatusTopic() const { return status_topic; }
    String getConnectionStatusTopic() const { return status_topic + "/connection"; }
    String getSystemStatusTopic() const { return status_topic + "/system"; }

    // ==================== 控制命令主题 (ctrl) ====================
    // 基础控制
    String getControlCommandTopic() const { return control_topic + "command"; }
    String getControlResponseTopic() const { return control_topic + "response"; }
    
    // 电源管理控制
    String getControlSleepTopic() const { return control_topic + "sleep"; }
    String getControlRebootTopic() const { return control_topic + "reboot"; }
    String getControlShutdownTopic() const { return control_topic + "shutdown"; }
    
    // 数据采集控制
    String getControlSamplingTopic() const { return control_topic + "sampling"; }
    String getControlReportingTopic() const { return control_topic + "reporting"; }
    
    // 设备功能控制
    String getControlLightTopic() const { return control_topic + "light"; }
    String getControlAlarmTopic() const { return control_topic + "alarm"; }
    String getControlLockTopic() const { return control_topic + "lock"; }

    // ==================== 配置主题 (config) ====================
    String getConfigUpdateTopic() const { return config_topic + "update"; }
    String getConfigRequestTopic() const { return config_topic + "request"; }
    String getConfigResponseTopic() const { return config_topic + "response"; }
    String getConfigMQTTTopic() const { return config_topic + "mqtt"; }
    String getConfigNetworkTopic() const { return config_topic + "network"; }
    String getConfigSensorTopic() const { return config_topic + "sensor"; }

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
        Serial.println("  Temperature: " + getTemperatureTopic());
        Serial.println("  Battery: " + getBatteryTopic());
        Serial.println();
        
        Serial.println("Control Topics:");
        Serial.println("  Command: " + getControlCommandTopic());
        Serial.println("  Sleep: " + getControlSleepTopic());
        Serial.println("  Reboot: " + getControlRebootTopic());
        Serial.println();
        
        Serial.println("Config Topics:");
        Serial.println("  Update: " + getConfigUpdateTopic());
        Serial.println("  MQTT: " + getConfigMQTTTopic());
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
