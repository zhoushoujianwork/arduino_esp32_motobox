#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include <Arduino.h>
#include "config.h"

// 调试级别定义
enum class DebugLevel {
    NONE = 0,    // 无调试输出
    ERROR = 1,   // 仅错误信息
    WARN = 2,    // 警告和错误
    INFO = 3,    // 重要信息、警告和错误
    DEBUG = 4,   // 所有调试信息
    VERBOSE = 5  // 详细调试信息（包括AT指令）
};

// 全局调试级别配置
#ifndef GLOBAL_DEBUG_LEVEL
#ifdef DEBUG
#define GLOBAL_DEBUG_LEVEL DebugLevel::DEBUG
#else
#define GLOBAL_DEBUG_LEVEL DebugLevel::INFO
#endif
#endif

// 模块特定调试级别
#ifndef AT_COMMAND_DEBUG_LEVEL
#define AT_COMMAND_DEBUG_LEVEL DebugLevel::DEBUG  // AT指令默认为DEBUG级别
#endif

#ifndef GNSS_DEBUG_LEVEL
#define GNSS_DEBUG_LEVEL DebugLevel::INFO  // GNSS默认为INFO级别
#endif

#ifndef MQTT_DEBUG_LEVEL
#define MQTT_DEBUG_LEVEL DebugLevel::INFO  // MQTT默认为INFO级别
#endif

// 调试宏定义 - 使用动态调试级别
#define DEBUG_ERROR(tag, msg, ...) \
    do { if ((int)DebugManager::getGlobalLevel() >= (int)DebugLevel::ERROR) { \
        Serial.printf("[ERROR][%s] " msg "\n", tag, ##__VA_ARGS__); \
    } } while(0)

#define DEBUG_WARN(tag, msg, ...) \
    do { if ((int)DebugManager::getGlobalLevel() >= (int)DebugLevel::WARN) { \
        Serial.printf("[WARN][%s] " msg "\n", tag, ##__VA_ARGS__); \
    } } while(0)

#define DEBUG_INFO(tag, msg, ...) \
    do { if ((int)DebugManager::getGlobalLevel() >= (int)DebugLevel::INFO) { \
        Serial.printf("[INFO][%s] " msg "\n", tag, ##__VA_ARGS__); \
    } } while(0)

#define DEBUG_DEBUG(tag, msg, ...) \
    do { if ((int)DebugManager::getGlobalLevel() >= (int)DebugLevel::DEBUG) { \
        Serial.printf("[DEBUG][%s] " msg "\n", tag, ##__VA_ARGS__); \
    } } while(0)

#define DEBUG_VERBOSE(tag, msg, ...) \
    do { if ((int)DebugManager::getGlobalLevel() >= (int)DebugLevel::VERBOSE) { \
        Serial.printf("[VERBOSE][%s] " msg "\n", tag, ##__VA_ARGS__); \
    } } while(0)

// AT指令专用调试宏 - 使用动态调试级别
#define AT_DEBUG_SEND(cmd) \
    do { if ((int)DebugManager::getATLevel() >= (int)DebugLevel::DEBUG) { \
        Serial.printf("[AT] >> %s\n", cmd); \
    } } while(0)

#define AT_DEBUG_RECV(resp) \
    do { if ((int)DebugManager::getATLevel() >= (int)DebugLevel::DEBUG) { \
        Serial.printf("[AT] << %s\n", resp); \
    } } while(0)

#define AT_VERBOSE_SEND(cmd) \
    do { if ((int)DebugManager::getATLevel() >= (int)DebugLevel::VERBOSE) { \
        Serial.printf("[AT] >> %s\n", cmd); \
    } } while(0)

#define AT_VERBOSE_RECV(resp) \
    do { if ((int)AT_COMMAND_DEBUG_LEVEL >= (int)DebugLevel::VERBOSE) { \
        Serial.printf("[AT] << %s\n", resp); \
    } } while(0)

// GNSS专用调试宏 - 使用动态调试级别
#define GNSS_INFO(msg, ...) \
    do { if ((int)DebugManager::getGNSSLevel() >= (int)DebugLevel::INFO) { \
        Serial.printf("[GNSS] " msg "\n", ##__VA_ARGS__); \
    } } while(0)

#define GNSS_DEBUG(msg, ...) \
    do { if ((int)DebugManager::getGNSSLevel() >= (int)DebugLevel::DEBUG) { \
        Serial.printf("[GNSS-DEBUG] " msg "\n", ##__VA_ARGS__); \
    } } while(0)

// MQTT专用调试宏 - 使用动态调试级别
#define MQTT_INFO(msg, ...) \
    do { if ((int)DebugManager::getMQTTLevel() >= (int)DebugLevel::INFO) { \
        Serial.printf("[MQTT] " msg "\n", ##__VA_ARGS__); \
    } } while(0)

#define MQTT_DEBUG(msg, ...) \
    do { if ((int)DebugManager::getMQTTLevel() >= (int)DebugLevel::DEBUG) { \
        Serial.printf("[MQTT-DEBUG] " msg "\n", ##__VA_ARGS__); \
    } } while(0)

// 运行时调试级别控制
class DebugManager {
public:
    static void setGlobalLevel(DebugLevel level);
    static void setATLevel(DebugLevel level);
    static void setGNSSLevel(DebugLevel level);
    static void setMQTTLevel(DebugLevel level);
    
    static DebugLevel getGlobalLevel();
    static DebugLevel getATLevel();
    static DebugLevel getGNSSLevel();
    static DebugLevel getMQTTLevel();
    
    static void printCurrentLevels();
    
private:
    static DebugLevel globalLevel;
    static DebugLevel atLevel;
    static DebugLevel gnssLevel;
    static DebugLevel mqttLevel;
};

#endif // DEBUG_UTILS_H
