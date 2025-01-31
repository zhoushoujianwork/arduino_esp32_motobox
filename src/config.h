#ifndef CONFIG_H
#define CONFIG_H

#define BTN_PIN 39

#define Enable_IMU 0
#define Enable_GPS 1
#define Enable_TFT 0
#define Enable_BLE 0
#define Enable_WIFI 1
// BLE服务和特征值的UUID
#define BLE_NAME "ESP32-MOTO"
#define SERVICE_UUID "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#define DEVICE_CHAR_UUID "BEB5483A-36E1-4688-B7F5-EA07361B26A8"
#define GPS_CHAR_UUID "BEB5483E-36E1-4688-B7F5-EA07361B26A8"
#define IMU_CHAR_UUID "BEB5483F-36E1-4688-B7F5-EA07361B26A8"

// TFT
#define TFT_CS 15
#define TFT_DC 2
#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_RST 4
#define TFT_BL 5
#define TFT_HOR_RES 172
#define TFT_VER_RES 320

// IMU配置
// 定义最大压弯角度
#define IMU_MAX_D 68 // 马奎斯 GP保持

// 定义最大速度
#define UI_MAX_SPEED 199 // 单位 km/h

#endif
