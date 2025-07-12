#ifndef CONFIG_H
#define CONFIG_H

/*
 * ESP32-S3 MotoBox Air780EG Configuration
 * 专用于Air780EG模块的配置文件
 */

#define ENABLE_GSM
#define USE_AIR780EG_GSM

// GPS功能通过Air780EG内置GNSS提供
#define ENABLE_GPS
#define USE_AIR780EG_GNSS

// 其他基础功能
#define ENABLE_COMPASS
#define ENABLE_IMU
#define ENABLE_AUDIO
#define ENABLE_LED
// #define ENABLE_TFT  // 暂时禁用TFT
#define ENABLE_BLE

// BLE配置
#define BLE_NAME                      "ESP32-MotoBox"
#define SERVICE_UUID        "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#define DEVICE_CHAR_UUID    "BEB5483A-36E1-4688-B7F5-EA07361B26A8"
#define GPS_CHAR_UUID       "BEB5483E-36E1-4688-B7F5-EA07361B26A8"
#define IMU_CHAR_UUID       "BEB5483F-36E1-4688-B7F5-EA07361B26A8"


// 音频配置
#define AUDIO_BOOT_SUCCESS_ENABLED    true
#define AUDIO_GPS_FIXED_ENABLED       true


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
#define MQTT_BROKER                  "222.186.32.152"
#define MQTT_PORT                    32571
#define MQTT_CLIENT_ID_PREFIX        ""
#define MQTT_USERNAME                "box"
#define MQTT_PASSWORD                "box"
#define MQTT_KEEPALIVE               60
#define MQTT_RECONNECT_INTERVAL      30000

// GPS配置
#define GPS_UPDATE_INTERVAL          1000
#define GPS_TIMEOUT                  10000

#endif // CONFIG_H
