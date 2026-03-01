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


#define ENABLE_LED
#define LED_DEBUG_ENABLED false // LED调试已禁用（优化性能）

// 轻量化版本 - 移除TFT功能，启用BLE功能
// #define ENABLE_TFT  // 轻量化版本不需要显示屏
#define ENABLE_BLE  // 启用蓝牙功能



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

// GNSS查询间隔配置（优化串口资源使用）
#define GNSS_UPDATE_INTERVAL_VALID    3000   // GNSS有效时3秒查询一次
#define GNSS_UPDATE_INTERVAL_INVALID  10000  // GNSS无效时10秒查询一次

// MQTT配置
#define MQTT_BROKER                  "daboluo.cc"
#define MQTT_PORT                    1883
#define MQTT_CLIENT_ID_PREFIX        ""
#define MQTT_USERNAME                "box"
#define MQTT_PASSWORD                "box"
#define MQTT_KEEPALIVE               60
#define MQTT_RECONNECT_INTERVAL      30000
#define MQTT_GPS_PUBLISH_INTERVAL     5000 // 5秒上报一次（正常模式默认值）
#define MQTT_DEVICE_STATUS_PUBLISH_INTERVAL 30000 // 30秒上报一次（正常模式默认值）

// 功耗模式管理
#define ENABLE_POWER_MODE_MANAGEMENT
#define POWER_MODE_AUTO_SWITCH_ENABLED    true    // 默认启用自动模式切换
#define POWER_MODE_EVALUATION_INTERVAL    5000    // 模式评估间隔（毫秒）
#define POWER_MODE_MIN_SWITCH_INTERVAL    30000   // 最小模式切换间隔（毫秒）

// GPS配置
#define GPS_UPDATE_INTERVAL          1000
#define GPS_TIMEOUT                  10000

// OTA升级配置
#define OTA_BATTERY_MIN_LEVEL        90   // 升级所需最低电池电量(%)
#define OTA_CHECK_INTERVAL           3600000  // 在线升级检查间隔(1小时)
#define OTA_DOWNLOAD_TIMEOUT         30000    // 下载超时时间(30秒)
#define OTA_MAX_RETRY_COUNT          3        // 最大重试次数
#define OTA_AUTO_UPGRADE_KEY         "ota_auto"  // 自动升级开关存储键
#define OTA_DEFAULT_AUTO_UPGRADE     true     // 默认启用自动升级

// 默认引脚定义（如果platformio.ini中未定义）
#ifndef BAT_PIN
#define BAT_PIN                      36   // 电池电压检测引脚
#endif

#ifndef CHARGING_STATUS_PIN
#define CHARGING_STATUS_PIN          2    // 充电状态检测引脚
#endif

#ifndef BUZZER_PIN
#define BUZZER_PIN                   25   // 蜂鸣器引脚
#endif

// IMU配置
#ifndef QMI8658_L_SLAVE_ADDRESS
#define QMI8658_L_SLAVE_ADDRESS     0x6B    // QMI8658 I2C地址
#endif

// 基础定位功能（默认启用）
#define ENABLE_GNSS_LOCATION        true    // 启用GNSS定位
#define ENABLE_FALLBACK_LOCATION    true    // 启用WiFi/LBS兜底定位

// MadgwickAHRS融合定位功能
// 注意：ENABLE_IMU_FUSION 在 platformio.ini 中通过编译参数定义
#ifdef ENABLE_IMU_FUSION
#define FUSION_LOCATION_UPDATE_INTERVAL  100     // 融合定位更新间隔（毫秒）
#define FUSION_LOCATION_DEBUG_ENABLED    true    // 启用调试输出
#define FUSION_LOCATION_INITIAL_LAT      39.9042 // 默认初始纬度（北京）
#define FUSION_LOCATION_INITIAL_LNG      116.4074// 默认初始经度（北京）
#define FUSION_LOCATION_PRINT_INTERVAL   5000    // 状态打印间隔（毫秒）

// MadgwickAHRS算法配置
#define MADGWICK_SAMPLE_RATE             100.0f  // 采样率（Hz）
#define MADGWICK_BETA                    0.1f    // 提高增益，改善响应速度

// 摩托车运动检测阈值
#define MOTO_ACCELERATION_THRESHOLD      1.0f    // 加速检测阈值（m/s²）
#define MOTO_BRAKING_THRESHOLD           -1.0f   // 制动检测阈值（m/s²）
#define MOTO_LEAN_THRESHOLD              0.175f  // 倾斜检测阈值（弧度，约10°）
#define MOTO_WHEELIE_THRESHOLD           0.35f   // 翘头检测阈值（弧度，约20°）
#define MOTO_STOPPIE_THRESHOLD           -0.35f  // 翘尾检测阈值（弧度，约-20°）
#define MOTO_DRIFT_THRESHOLD             3.0f    // 漂移检测阈值（m/s²）

// 零速检测配置
#define ZUPT_SPEED_THRESHOLD             0.3f    // 降低阈值
#define ZUPT_TIME_THRESHOLD              1000    // 零速持续时间阈值（毫秒）
#endif // ENABLE_IMU_FUSION

// 电池管理优化配置
#define BATTERY_VOLTAGE_FILTER_ENABLED    true    // 启用电压滤波
#define BATTERY_ADC_SAMPLE_COUNT          8       // ADC采样次数
#define BATTERY_ADC_SAMPLE_DELAY          5       // ADC采样间隔（毫秒）
#define BATTERY_VOLTAGE_THRESHOLD         100     // 电压变化阈值（毫伏）
#define BATTERY_CALIBRATION_ENABLED       true    // 启用电压校准
#define BATTERY_DEBUG_ENABLED             false   // 电池调试输出

// 数据采集调试配置
#define DATA_COLLECTOR_DEBUG_ENABLED      true    // 数据采集调试输出
#define DATA_COLLECTOR_VERBOSE_ENABLED    false   // 数据采集详细输出
#define DATA_COLLECTOR_OUTPUT_INTERVAL    5000    // 数据采集输出间隔（毫秒）

// BLE配置 - 模块化特征值设计
#ifdef ENABLE_BLE
#define BLE_DEVICE_NAME_PREFIX            "MotoBox-"          // BLE设备名称前缀

// 服务UUID
#define BLE_SERVICE_UUID                  "A7B3C4D5-E6F7-4A8B-9C0D-1E2F3A4B5C6D"

// 模块化特征值UUID
#define BLE_CHAR_GPS_UUID                 "B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E"  // GPS位置数据特征值
#define BLE_CHAR_IMU_UUID                 "C9D0E1F2-A3B4-5C6D-7E8F-9A0B1C2D3E4F"  // IMU传感器数据特征值
#define BLE_CHAR_COMPASS_UUID             "D0E1F2A3-B4C5-6D7E-8F9A-0B1C2D3E4F5A"  // 罗盘数据特征值
#define BLE_CHAR_SYSTEM_UUID              "E1F2A3B4-C5D6-7E8F-9A0B-1C2D3E4F5A6B"  // 系统状态数据特征值

// 更新频率配置
#define BLE_GPS_UPDATE_INTERVAL           1000                // GPS数据更新间隔（毫秒，1Hz，1秒一次）
#define BLE_IMU_UPDATE_INTERVAL           200                 // IMU数据更新间隔（毫秒，5Hz，200ms一次）
#define BLE_COMPASS_UPDATE_INTERVAL       500                 // 罗盘数据更新间隔（毫秒，2Hz）
#define BLE_SYSTEM_UPDATE_INTERVAL        10000               // 系统状态更新间隔（毫秒，0.1Hz，10秒一次）
#define BLE_UPDATE_INTERVAL               200                 // 通用BLE数据更新间隔（毫秒，向后兼容）

#define BLE_DEBUG_ENABLED                 false               // BLE调试输出
#endif

#endif // CONFIG_H
