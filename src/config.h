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

// 根据编译环境确定使用哪个GSM模块
#ifdef ENABLE_AIR780EG
#define USE_AIR780EG_GSM
#elif defined(ENABLE_ML307)
#define USE_ML307_GSM
#else
// 默认使用ML307（向后兼容）
#define USE_ML307_GSM
#endif

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
#define APP_NAME            "MOTO-BOX"
#define BLE_NAME            "MOTO-BOX"
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
#define MQTT_USER         "box"
#define MQTT_PASSWORD     "box"
#define MQTT_KEEP_ALIVE   60

/* 
TFT 配置请在lib/TFT_eSPI/User_Setup_Select.h中选择
*/

/* Display Configuration */
#define TFT_HOR_RES        172
#define TFT_VER_RES        320
#define TFT_ROTATION       1 // 0: 0度, 1: 90度, 2: 180度, 3: 270度
#define UI_MAX_SPEED       199    /* Maximum speed in km/h */

#ifdef ENABLE_SDCARD
// SPI模式SD卡引脚配置（在platformio.ini中定义）
#ifndef SD_MODE_SPI
// 默认使用SDIO模式
#define SD_MMC_MODE
#endif
#endif

/* Audio Configuration */
// 音频引脚配置检查
#if defined(IIS_S_WS_PIN) && defined(IIS_S_BCLK_PIN) && defined(IIS_S_DATA_PIN)
#define ENABLE_AUDIO
#endif

/* Audio Events Configuration */
#define AUDIO_BOOT_SUCCESS_ENABLED    true
#define AUDIO_WIFI_CONNECTED_ENABLED  true
#define AUDIO_GPS_FIXED_ENABLED       true
#define AUDIO_LOW_BATTERY_ENABLED     true
#define AUDIO_SLEEP_MODE_ENABLED      true

/* Debug Configuration */
// MQTT调试输出控制
#ifndef MQTT_DEBUG_ENABLED
#define MQTT_DEBUG_ENABLED            false
#endif

// 网络调试输出控制
#ifndef NETWORK_DEBUG_ENABLED
#define NETWORK_DEBUG_ENABLED         false
#endif

// 系统调试输出控制
#ifndef SYSTEM_DEBUG_ENABLED
#define SYSTEM_DEBUG_ENABLED          true
#endif

// GPS全链路调试输出控制
#ifndef GPS_DEBUG_ENABLED
#define GPS_DEBUG_ENABLED             true
#endif

// AT指令调试输出控制
#ifndef AT_COMMAND_DEBUG_ENABLED
#define AT_COMMAND_DEBUG_ENABLED      false  // 默认关闭AT指令调试
#endif

// 调试级别配置 (0=无, 1=错误, 2=警告, 3=信息, 4=调试, 5=详细)
#ifndef GLOBAL_DEBUG_LEVEL
#ifdef DEBUG
#define GLOBAL_DEBUG_LEVEL            4  // 调试版本默认为DEBUG级别
#else
#define GLOBAL_DEBUG_LEVEL            3  // 发布版本默认为INFO级别
#endif
#endif

#ifndef AT_COMMAND_DEBUG_LEVEL
#define AT_COMMAND_DEBUG_LEVEL        4  // AT指令调试级别
#endif

#ifndef GNSS_DEBUG_LEVEL
#define GNSS_DEBUG_LEVEL              4  // GNSS调试级别
#endif

#ifndef MQTT_DEBUG_LEVEL
#define MQTT_DEBUG_LEVEL              4  // MQTT调试级别
#endif

// 智能定位切换配置
#ifndef SMART_LOCATION_ENABLED
#define SMART_LOCATION_ENABLED        true   // 启用智能定位切换
#endif

#ifndef GNSS_FAILURE_TIMEOUT_MS
#define GNSS_FAILURE_TIMEOUT_MS       30000  // GNSS失败30秒后启用LBS备用
#endif

#ifndef GNSS_RECOVERY_TIMEOUT_MS
#define GNSS_RECOVERY_TIMEOUT_MS      60000  // GNSS恢复60秒后停用LBS备用
#endif

#ifndef LBS_FALLBACK_INTERVAL_MS
#define LBS_FALLBACK_INTERVAL_MS      30000  // LBS备用模式下的更新间隔
#endif

// GNSS调试输出控制（Air780EG GNSS功能）
#ifndef GNSS_DEBUG_ENABLED
#define GNSS_DEBUG_ENABLED            true
#endif

// LBS调试输出控制（基站定位功能）
#ifndef LBS_DEBUG_ENABLED
#define LBS_DEBUG_ENABLED             true
#endif

#endif /* CONFIG_H */
