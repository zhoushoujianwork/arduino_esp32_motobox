#ifndef CONFIG_H
#define CONFIG_H

/*
 * Device Operation Mode Configuration
 * Modes are now defined in platformio.ini build_flags:
 * 1. MODE_ALLINONE: Standalone mode without Bluetooth, direct data collection and display
 * 2. MODE_SERVER: Master mode - collects sensor data, sends via Bluetooth to client,
 *           and can send data to server via MQTT over WiFi
 * 3. MODE_CLIENT: Slave mode - receives data from server via Bluetooth and displays it
 */

/* 是否启用睡眠模式，在platformio.ini中定义 
 */

// GSM引脚配置检查
#if defined(GSM_RX_PIN) && defined(GSM_TX_PIN)
#define ENABLE_GSM
#else
#define ENABLE_WIFI
#endif

// GPS引脚配置检查
#if defined(GPS_RX_PIN) && defined(GPS_TX_PIN)
#define ENABLE_GPS
#endif


// 罗盘引脚配置检查
#if defined(GPS_COMPASS_SDA) && defined(GPS_COMPASS_SCL)
#define ENABLE_COMPASS
#endif

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "unknown"
#endif

#ifndef HARDWARE_VERSION
#define HARDWARE_VERSION "unknown"
#endif

/*
 * 以下定义保留是为了兼容性，实际的值在 platformio.ini 中配置
 * Common device configurations are now defined in platformio.ini [env] section
 * Pin configurations are defined in respective environment sections [env:allinone], [env:server], [env:client]
 */

/* Common Device Configuration */
#define APP_NAME            "DBLBOX"
#define BLE_NAME            "DBLBOX"
#define SERVICE_UUID        "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#define DEVICE_CHAR_UUID    "BEB5483A-36E1-4688-B7F5-EA07361B26A8"
#define GPS_CHAR_UUID       "BEB5483E-36E1-4688-B7F5-EA07361B26A8"
#define IMU_CHAR_UUID       "BEB5483F-36E1-4688-B7F5-EA07361B26A8"

/* LED Configuration */
#define LED_BLINK_INTERVAL 100

/* IMU Configuration */
#if defined(IMU_SDA_PIN) && defined(IMU_SCL_PIN)
#define ENABLE_IMU  // 默认启用IMU
#endif

#define IMU_MAX_D          68     /* Marquis GP retention */


/* MQTT Configuration */
#define MQTT_BROKER        "mq-hub.daboluo.cc"
// #define MQTT_SERVER        "222.186.32.152"
#define MQTT_PORT         32571
#define MQTT_USER         ""
#define MQTT_PASSWORD     ""
#define MQTT_KEEP_ALIVE   60

/* 
TFT 配置请在lib/TFT_eSPI/User_Setup_Select.h中选择
*/

/* Display Configuration */
#define TFT_HOR_RES        172
#define TFT_VER_RES        320
#define TFT_ROTATION       1 // 0: 0度, 1: 90度, 2: 180度, 3: 270度
#define UI_MAX_SPEED       199    /* Maximum speed in km/h */

// GNSS和LBS默认配置
#ifdef ENABLE_GSM
#define ENABLE_GNSS_BY_DEFAULT false   // 默认启用GNSS
#define ENABLE_LBS_BY_DEFAULT true    // 默认启用LBS
#endif

#endif /* CONFIG_H */
