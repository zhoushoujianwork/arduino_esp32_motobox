#ifndef CONFIG_H
#define CONFIG_H

#define BTN_PIN 39

// IMU
#define Enable_IMU 0
#define IMU_PIN 33
#define IMU_MAX_D 68 // 马奎斯 GP保持

// GPS
#define Enable_GPS 0

// BLE
#define Enable_BLE 0 // BLE服务和特征值的UUID
#define BLE_NAME "ESP32-MOTO"
#define SERVICE_UUID "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#define DEVICE_CHAR_UUID "BEB5483A-36E1-4688-B7F5-EA07361B26A8"
#define GPS_CHAR_UUID "BEB5483E-36E1-4688-B7F5-EA07361B26A8"
#define IMU_CHAR_UUID "BEB5483F-36E1-4688-B7F5-EA07361B26A8"

// WIFI
#define Enable_WIFI 1

// TFT
#define Enable_TFT 1
#define TFT_CS 15
#define TFT_DC 2
#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_RST 4
#define TFT_BL 5
#define TFT_HOR_RES 172
#define TFT_VER_RES 320
#define UI_MAX_SPEED 199 // 单位 km/h

// 电池
#define Enable_BAT 0
#define BAT_PIN 7
#define BAT_MIN_VOLTAGE 2900
#define BAT_MAX_VOLTAGE 3300

// MQTT Configuration
#define MQTT_SERVER "your-emqx-server.com"
#define MQTT_PORT 1883
#define MQTT_USER "your-username"
#define MQTT_PASSWORD "your-password"
#define MQTT_CLIENT_ID "device-001" // 确保每个设备使用唯一的客户端ID
#define MQTT_TOPIC_GPS "esp32/gps"
#define MQTT_TOPIC_IMU "esp32/imu"

#endif
