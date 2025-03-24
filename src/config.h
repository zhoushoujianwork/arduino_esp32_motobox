#ifndef CONFIG_H
#define CONFIG_H

/* Version Information */
#define VERSION "2.0.0"

/*
 * Device Operation Mode Configuration
 * Modes are now defined in platformio.ini build_flags:
 * 1. MODE_ALLINONE: Standalone mode without Bluetooth, direct data collection and display
 * 2. MODE_SERVER: Master mode - collects sensor data, sends via Bluetooth to client,
 *           and can send data to server via MQTT over WiFi
 * 3. MODE_CLIENT: Slave mode - receives data from server via Bluetooth and displays it
 */

/*
 * 以下定义保留是为了兼容性，实际的值在 platformio.ini 中配置
 * Common device configurations are now defined in platformio.ini [env] section
 * Pin configurations are defined in respective environment sections [env:allinone], [env:server], [env:client]
 */

/* Common Device Configuration */
#ifndef BLE_NAME
#define BLE_NAME            "ESP32-MOTO"
#endif

#ifndef SERVICE_UUID
#define SERVICE_UUID        "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#endif

#ifndef DEVICE_CHAR_UUID
#define DEVICE_CHAR_UUID    "BEB5483A-36E1-4688-B7F5-EA07361B26A8"
#endif

#ifndef GPS_CHAR_UUID
#define GPS_CHAR_UUID       "BEB5483E-36E1-4688-B7F5-EA07361B26A8"
#endif

#ifndef IMU_CHAR_UUID
#define IMU_CHAR_UUID       "BEB5483F-36E1-4688-B7F5-EA07361B26A8"
#endif

/* Battery Configuration */
#ifndef BAT_MIN_VOLTAGE
#define BAT_MIN_VOLTAGE     2900
#endif

#ifndef BAT_MAX_VOLTAGE
#define BAT_MAX_VOLTAGE     3250
#endif

/* Display Configuration */
#ifndef TFT_HOR_RES
#define TFT_HOR_RES        172
#endif

#ifndef TFT_VER_RES
#define TFT_VER_RES        320
#endif

#ifndef UI_MAX_SPEED
#define UI_MAX_SPEED       199    /* Maximum speed in km/h */
#endif

/* LED Configuration */
#ifndef LED_BLINK_INTERVAL
#define LED_BLINK_INTERVAL 100
#endif

/* IMU Configuration */
#ifndef IMU_MAX_D
#define IMU_MAX_D          68     /* Marquis GP retention */
#endif

/* IMU Interrupt Configuration */
#ifndef IMU_INT1_PIN
#define IMU_INT1_PIN       1      /* GPIO1 used for IMU interrupt */
#endif

/* GPS Configuration */
#ifndef GPS_DEFAULT_BAUDRATE
#define GPS_DEFAULT_BAUDRATE 9600
#endif

#ifndef GPS_HZ
#define GPS_HZ              2      /* Supported: 1, 2, 5, 10 Hz */
#endif

/* MQTT Configuration */
#ifndef MQTT_SERVER
#define MQTT_SERVER        "mq-hub.daboluo.cc"
#endif

#ifndef MQTT_PORT
#define MQTT_PORT         1883
#endif

#ifndef MQTT_USER
#define MQTT_USER         ""
#endif

#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD     ""
#endif

/* MQTT Topics - These must stay in C++ code because they include runtime expressions */
#define MQTT_TOPIC_DEVICE_INFO  String("vehicle/v1/") + device.get_device_id() + "/device/info"
#define MQTT_TOPIC_GPS         String("vehicle/v1/") + device.get_device_id() + "/gps/position"
#define MQTT_TOPIC_IMU         String("vehicle/v1/") + device.get_device_id() + "/imu/gyro"

/* MQTT Intervals (in milliseconds) */
#ifndef MQTT_DEVICE_INFO_INTERVAL
#define MQTT_DEVICE_INFO_INTERVAL  5000
#endif

#ifndef MQTT_IMU_PUBLISH_INTERVAL
#define MQTT_IMU_PUBLISH_INTERVAL   200
#endif

#ifndef MQTT_BAT_PRINT_INTERVAL
#define MQTT_BAT_PRINT_INTERVAL   10000
#endif

#endif /* CONFIG_H */
