#ifndef CONFIG_H
#define CONFIG_H
#define VERSION "2.0.0"

// 设置服务运行模式，目前支持 3 种配置
// 1. allinOne的方式，不需要用到蓝牙，直接自己采集数据渲染到屏幕；
// 2.
// server模式，作为主机方式负责采集传感器数据并通过蓝牙发送给从机屏幕，主机还能通过WIFI
// 配置将后台数据实时 mqtt发送到服务器；
// 3. client模式，作为从机方式负责接收主机蓝牙发送过来的数据，并渲染到屏幕上；

#define MODE_ALLINONE
// #define MODE_SERVER
// #define MODE_CLIENT

#ifdef MODE_ALLINONE
#define BTN_PIN 39
#define BAT_PIN 20
#define BAT_MIN_VOLTAGE 2900
#define BAT_MAX_VOLTAGE 3250
#define IMU_SDA_PIN 42
#define IMU_SCL_PIN 2
#define LED_PIN 8

#define TFT_CS 6
#define TFT_DC 4
#define TFT_MOSI 15
#define TFT_SCLK 5
#define TFT_RST 7
#define TFT_BL 16

#endif

#ifdef MODE_SERVER
#define BLE_SERVER // 启用服务端模式

#define IMU_SDA_PIN 42
#define IMU_SCL_PIN 2
#define BTN_PIN 39
#define BAT_PIN 7
#define BAT_MIN_VOLTAGE 2900
#define BAT_MAX_VOLTAGE 3250
// 添加LED配置
#define LED_PIN 8
#endif

#ifdef MODE_CLIENT
#define BLE_CLIENT // 启用客户端模式

#define BAT_PIN 14
#define BAT_MIN_VOLTAGE 2900
#define BAT_MAX_VOLTAGE 3250
#define LED_PIN 3

#define TFT_CS 15
#define TFT_DC 2
#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_RST 4
#define TFT_BL 5
#endif

#define IMU_MAX_D 68 // 马奎斯 GP保持
#define BLE_NAME "ESP32-MOTO"
#define SERVICE_UUID "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#define DEVICE_CHAR_UUID "BEB5483A-36E1-4688-B7F5-EA07361B26A8"
#define GPS_CHAR_UUID "BEB5483E-36E1-4688-B7F5-EA07361B26A8"
#define IMU_CHAR_UUID "BEB5483F-36E1-4688-B7F5-EA07361B26A8"


#define TFT_HOR_RES 172
#define TFT_VER_RES 320

#define UI_MAX_SPEED 199 // 单位 km/h
#define LED_BLINK_INTERVAL 100

// MQTT Configuration
#define MQTT_SERVER "mq-hub.daboluo.cc"
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PASSWORD ""
#define MQTT_TOPIC_DEVICE_INFO                                                 \
  String("vehicle/v1/") + device.get_device_id() + "/device/info"
#define MQTT_TOPIC_GPS                                                         \
  String("vehicle/v1/") + device.get_device_id() + "/gps/position"
#define MQTT_TOPIC_IMU                                                         \
  String("vehicle/v1/") + device.get_device_id() + "/imu/gyro"



// 添加GPS配置
#define GPS_RX_PIN 12
#define GPS_TX_PIN 11
#define GPS_DEFAULT_BAUDRATE 9600
#define GPS_UPDATE_INTERVAL 1000



// 添加MQTT发布间隔配置
#define MQTT_DEVICE_INFO_INTERVAL 5000
#define MQTT_IMU_PUBLISH_INTERVAL 200
#define MQTT_BAT_PRINT_INTERVAL 10000

#endif
