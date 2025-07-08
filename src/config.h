#ifndef CONFIG_H
#define CONFIG_H

/*
 * ESP32-S3 MotoBox Air780EG Configuration
 * 专用于Air780EG模块的配置文件
 */

// 强制启用Air780EG GSM模块
#define ENABLE_GSM
#define USE_AIR780EG_GSM

// GPS功能通过Air780EG内置GNSS提供
#define ENABLE_GPS
#define USE_AIR780EG_GNSS

// MQTT功能完全禁用，专注于GPS功能开发
// #define ENABLE_MQTT
// #define USE_AIR780EG_MQTT
#define DISABLE_MQTT

// 其他基础功能
#define ENABLE_COMPASS
#define ENABLE_IMU
#define ENABLE_AUDIO
#define ENABLE_LED
// #define ENABLE_TFT  // 暂时禁用TFT
#define ENABLE_BLE

// BLE配置
#define BLE_NAME                      "ESP32-MotoBox"
#define SERVICE_UUID                  "12345678-1234-1234-1234-123456789abc"
#define DEVICE_CHAR_UUID              "87654321-4321-4321-4321-cba987654321"
#define GPS_CHAR_UUID                 "11111111-2222-3333-4444-555555555555"
#define IMU_CHAR_UUID                 "66666666-7777-8888-9999-aaaaaaaaaaaa"

// 调试配置
#define GPS_DEBUG_ENABLED             false
#define GNSS_DEBUG_ENABLED            false
#define LBS_DEBUG_ENABLED             false

// 音频配置
#define AUDIO_BOOT_SUCCESS_ENABLED    true
#define AUDIO_LOW_BATTERY_ENABLED     true
#define AUDIO_GPS_FIXED_ENABLED       true

// MQTT配置
#define MQTT_USER                     "motobox"
#define MQTT_KEEP_ALIVE               60
// 避免与PubSubClient库冲突，使用不同的宏名  
#define LBS_DEBUG_ENABLED             false
#define MQTT_DEBUG_ENABLED            true
#define NETWORK_DEBUG_ENABLED         true

// 调试宏定义
#if GPS_DEBUG_ENABLED
#define GPS_DEBUG_PRINTLN(x)          Serial.println(x)
#define GPS_DEBUG_PRINTF(fmt, ...)    Serial.printf(fmt, ##__VA_ARGS__)
#else
#define GPS_DEBUG_PRINTLN(x)
#define GPS_DEBUG_PRINTF(fmt, ...)
#endif

#if GNSS_DEBUG_ENABLED
#define GNSS_DEBUG_PRINTLN(x)         Serial.println(x)
#define GNSS_DEBUG_PRINTF(fmt, ...)   Serial.printf(fmt, ##__VA_ARGS__)
#else
#define GNSS_DEBUG_PRINTLN(x)
#define GNSS_DEBUG_PRINTF(fmt, ...)
#endif

#if MQTT_DEBUG_ENABLED
#define MQTT_DEBUG_PRINTLN(x)         Serial.println(x)
#define MQTT_DEBUG_PRINTF(fmt, ...)   Serial.printf(fmt, ##__VA_ARGS__)
#else
#define MQTT_DEBUG_PRINTLN(x)
#define MQTT_DEBUG_PRINTF(fmt, ...)
#endif

#if NETWORK_DEBUG_ENABLED
#define NETWORK_DEBUG_PRINTLN(x)      Serial.println(x)
#define NETWORK_DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
#define NETWORK_DEBUG_PRINTLN(x)
#define NETWORK_DEBUG_PRINTF(fmt, ...)
#endif

// 版本信息
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "unknown"
#endif

#ifndef HARDWARE_VERSION
#define HARDWARE_VERSION "esp32-air780eg"
#endif

#ifndef BUILD_NUMBER
#define BUILD_NUMBER 0
#endif

// Air780EG配置
#define AIR780EG_BAUD_RATE           115200
#define AIR780EG_TIMEOUT             5000
#define AIR780EG_GNSS_UPDATE_RATE    1000  // 1Hz
#define AIR780EG_NETWORK_CHECK_INTERVAL 5000  // 5秒

// MQTT配置
#define MQTT_BROKER                  "your-mqtt-broker.com"
#define MQTT_PORT                    1883
#define MQTT_CLIENT_ID_PREFIX        "motobox_"
#define MQTT_USERNAME                ""
#define MQTT_PASSWORD                ""
#define MQTT_KEEPALIVE               60
#define MQTT_RECONNECT_INTERVAL      30000

// GPS配置
#define GPS_UPDATE_INTERVAL          1000
#define GPS_TIMEOUT                  10000

#endif // CONFIG_H
