#include "DebugUtils.h"

// 静态成员变量定义
DebugLevel DebugManager::globalLevel = (DebugLevel)GLOBAL_DEBUG_LEVEL;
DebugLevel DebugManager::atLevel = (DebugLevel)AT_COMMAND_DEBUG_LEVEL;
DebugLevel DebugManager::gnssLevel = (DebugLevel)GNSS_DEBUG_LEVEL;
DebugLevel DebugManager::mqttLevel = (DebugLevel)MQTT_DEBUG_LEVEL;

void DebugManager::setGlobalLevel(DebugLevel level) {
    globalLevel = level;
    Serial.printf("[DebugManager] 全局调试级别设置为: %d\n", (int)level);
}

void DebugManager::setATLevel(DebugLevel level) {
    atLevel = level;
    Serial.printf("[DebugManager] AT指令调试级别设置为: %d\n", (int)level);
}

void DebugManager::setGNSSLevel(DebugLevel level) {
    gnssLevel = level;
    Serial.printf("[DebugManager] GNSS调试级别设置为: %d\n", (int)level);
}

void DebugManager::setMQTTLevel(DebugLevel level) {
    mqttLevel = level;
    Serial.printf("[DebugManager] MQTT调试级别设置为: %d\n", (int)level);
}

DebugLevel DebugManager::getGlobalLevel() {
    return globalLevel;
}

DebugLevel DebugManager::getATLevel() {
    return atLevel;
}

DebugLevel DebugManager::getGNSSLevel() {
    return gnssLevel;
}

DebugLevel DebugManager::getMQTTLevel() {
    return mqttLevel;
}

void DebugManager::printCurrentLevels() {
    Serial.println("=== 当前调试级别配置 ===");
    Serial.printf("全局级别: %d\n", (int)globalLevel);
    Serial.printf("AT指令级别: %d\n", (int)atLevel);
    Serial.printf("GNSS级别: %d\n", (int)gnssLevel);
    Serial.printf("MQTT级别: %d\n", (int)mqttLevel);
    Serial.println("级别说明: 0=无, 1=错误, 2=警告, 3=信息, 4=调试, 5=详细");
    Serial.println("========================");
}
