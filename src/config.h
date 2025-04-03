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

/* 是否启用睡眠模式，在platformio.ini中定义 
 * 默认值: ENABLE_SLEEP=1 (启用)
 * 禁用值: ENABLE_SLEEP=0 (禁用)
 */
#ifndef ENABLE_SLEEP
#define ENABLE_SLEEP 0 
#endif

/*
 * 以下定义保留是为了兼容性，实际的值在 platformio.ini 中配置
 * Common device configurations are now defined in platformio.ini [env] section
 * Pin configurations are defined in respective environment sections [env:allinone], [env:server], [env:client]
 */

/* Common Device Configuration */
#define BLE_NAME            "ESP32-MOTO"
#define SERVICE_UUID        "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#define DEVICE_CHAR_UUID    "BEB5483A-36E1-4688-B7F5-EA07361B26A8"
#define GPS_CHAR_UUID       "BEB5483E-36E1-4688-B7F5-EA07361B26A8"
#define IMU_CHAR_UUID       "BEB5483F-36E1-4688-B7F5-EA07361B26A8"

/* Battery Configuration */
#define BAT_MIN_VOLTAGE     2900
#define BAT_MAX_VOLTAGE     3250

/* LED Configuration */
#define LED_BLINK_INTERVAL 100

/* IMU Configuration */
#define IMU_MAX_D          68     /* Marquis GP retention */
#define IMU_INT1_PIN       1      /* GPIO1 used for IMU interrupt */

/* MQTT Configuration */
#define MQTT_SERVER        "mq-hub.daboluo.cc"
#define MQTT_PORT         32571
#define MQTT_USER         ""
#define MQTT_PASSWORD     ""

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

/* 
TFT 配置请在lib/TFT_eSPI/User_Setup_Select.h中选择
*/

/* Display Configuration */
#define TFT_HOR_RES        172
#define TFT_VER_RES        320
#define TFT_ROTATION       1 // 0: 0度, 1: 90度, 2: 180度, 3: 270度
#define UI_MAX_SPEED       199    /* Maximum speed in km/h */

#endif /* CONFIG_H */
