#ifndef CONFIG_H
#define CONFIG_H

/* Version Information */
#define VERSION "2.0.0"

/*
 * Device Operation Mode Configuration
 * Supported modes:
 * 1. MODE_ALLINONE: Standalone mode without Bluetooth, direct data collection and display
 * 2. SERVER: Master mode - collects sensor data, sends via Bluetooth to client,
 *           and can send data to server via MQTT over WiFi
 * 3. CLIENT: Slave mode - receives data from server via Bluetooth and displays it
 */

/* Common Device Configuration */
#define BLE_NAME            "ESP32-MOTO"
#define SERVICE_UUID        "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#define DEVICE_CHAR_UUID    "BEB5483A-36E1-4688-B7F5-EA07361B26A8"
#define GPS_CHAR_UUID       "BEB5483E-36E1-4688-B7F5-EA07361B26A8"
#define IMU_CHAR_UUID       "BEB5483F-36E1-4688-B7F5-EA07361B26A8"
#define COMPASS_CHAR_UUID   "BEB5483G-36E1-4688-B7F5-EA07361B26A8"

/* Battery Configuration */
#define BAT_MIN_VOLTAGE     2900
#define BAT_MAX_VOLTAGE     3250

/* Display Configuration */
#define TFT_HOR_RES        172
#define TFT_VER_RES        320
#define UI_MAX_SPEED       199    /* Maximum speed in km/h */

/* LED Configuration */
#define LED_BLINK_INTERVAL 100

/* IMU Configuration */
#define IMU_MAX_D          68     /* Marquis GP retention */

/* Compass Configuration */
#define COMPASS_DECLINATION 2.5  /* 磁偏角，单位：度，可根据所在地区调整 */

/* GPS Configuration */
#define GPS_DEFAULT_BAUDRATE 9600
#define GPS_HZ              2      /* Supported: 1, 2, 5, 10 Hz */

/* MQTT Configuration */
#define MQTT_SERVER        "mq-hub.daboluo.cc"
#define MQTT_PORT         1883
#define MQTT_USER         ""
#define MQTT_PASSWORD     ""

/* MQTT Topics */
#define MQTT_TOPIC_DEVICE_INFO  String("vehicle/v1/") + device.get_device_id() + "/device/info"
#define MQTT_TOPIC_GPS         String("vehicle/v1/") + device.get_device_id() + "/gps/position"
#define MQTT_TOPIC_IMU         String("vehicle/v1/") + device.get_device_id() + "/imu/gyro"
#define MQTT_TOPIC_COMPASS     String("vehicle/v1/") + device.get_device_id() + "/compass/heading"

/* MQTT Intervals (in milliseconds) */
#define MQTT_DEVICE_INFO_INTERVAL  5000
#define MQTT_IMU_PUBLISH_INTERVAL   200
#define MQTT_BAT_PRINT_INTERVAL   10000
#define MQTT_COMPASS_PUBLISH_INTERVAL 500

/* 配置时钟频率宏 */
#define GPS_PUBLISH_INTERVAL 1000  // 1Hz
#define IMU_PUBLISH_INTERVAL 500   // 2Hz
#define BLE_PUBLISH_INTERVAL 1000  // 1Hz

/* 模式定义：通过platformio.ini中的build_flags自动定义，这里不需要再定义引脚 */
#ifdef MODE_SERVER
    #define BLE_SERVER              /* Enable server mode */
#endif

#ifdef MODE_CLIENT
    #define BLE_CLIENT              /* Enable client mode */
#endif

#endif /* CONFIG_H */
