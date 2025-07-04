#include "device.h"
#include "tft/TFT.h"
#include "gps/GPSManager.h"
#include "imu/qmi8658.h"

#ifdef ENABLE_SDCARD
#include "SD/SDManager.h"
extern SDManager sdManager;
#endif

#ifdef ENABLE_AUDIO
extern AudioManager audioManager;
#endif

extern const VersionInfo &getVersionInfo();

device_state_t device_state;
state_changes_t state_changes;

void print_device_info()
{
    // 如果休眠倒计时的时候不打印
    if (powerManager.powerState == POWER_STATE_COUNTDOWN)
    {
        return;
    }
    Serial.println("Device Info:");
    Serial.printf("Device ID: %s\n", device_state.device_id.c_str());
    Serial.printf("Sleep Time: %d\n", device_state.sleep_time);
    Serial.printf("Firmware Version: %s\n", device_state.device_firmware_version);
    Serial.printf("Hardware Version: %s\n", device_state.device_hardware_version);
    Serial.printf("WiFi Connected: %d\n", device_state.wifiConnected);
    Serial.printf("BLE Connected: %d\n", device_state.bleConnected);
    Serial.printf("Battery Voltage: %d\n", device_state.battery_voltage);
    Serial.printf("Battery Percentage: %d\n", device_state.battery_percentage);
    Serial.printf("Is Charging: %d\n", device_state.is_charging);
    Serial.printf("External Power: %d\n", device_state.external_power);
    Serial.printf("GPS Ready: %d\n", device_state.gpsReady);
    Serial.printf("GPS Type: %s\n", gpsManager.getType().c_str());
    Serial.printf("IMU Ready: %d\n", device_state.imuReady);
    Serial.printf("Compass Ready: %d\n", device_state.compassReady);
    Serial.printf("SD Card Ready: %d\n", device_state.sdCardReady);
    if (device_state.sdCardReady) {
        Serial.printf("SD Card Size: %llu MB\n", device_state.sdCardSizeMB);
        Serial.printf("SD Card Free: %llu MB\n", device_state.sdCardFreeMB);
    }
    Serial.printf("Audio Ready: %d\n", device_state.audioReady);
    Serial.println("--------------------------------");
}

String Device::get_device_id()
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char device_id[13];
    snprintf(device_id, sizeof(device_id), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(device_id);
}

device_state_t *get_device_state()
{
    return &device_state;
}

void set_device_state(device_state_t *state)
{
    device_state = *state;
}

// 生成精简版设备状态JSON
// fw: 固件版本, hw: 硬件版本, wifi/ble/gps/imu/compass: 各模块状态, bat_v: 电池电压, bat_pct: 电池百分比, is_charging: 充电状态, ext_power: 外部电源状态, sd: SD卡状态
String device_state_to_json(device_state_t *state)
{
    StaticJsonDocument<256> doc; // 精简后更小即可
    doc["fw"] = device_state.device_firmware_version;
    doc["hw"] = device_state.device_hardware_version;
    doc["wifi"] = device_state.wifiConnected;
    doc["ble"] = device_state.bleConnected;
    doc["gps"] = device_state.gpsReady;
    doc["gps_type"] = gpsManager.getType();
    doc["imu"] = device_state.imuReady;
    doc["compass"] = device_state.compassReady;
    doc["bat_v"] = device_state.battery_voltage;
    doc["bat_pct"] = device_state.battery_percentage;
    doc["is_charging"] = device_state.is_charging;
    doc["ext_power"] = device_state.external_power;
    doc["sd"] = device_state.sdCardReady;
    if (device_state.sdCardReady) {
        doc["sd_size"] = device_state.sdCardSizeMB;
        doc["sd_free"] = device_state.sdCardFreeMB;
    }
    doc["audio"] = device_state.audioReady;
    return doc.as<String>();
}

void mqttMessageCallback(const String &topic, const String &payload)
{
#ifndef DISABLE_MQTT
    Serial.printf("收到消息 [%s]: %s\n", topic.c_str(), payload.c_str());

    // 解析JSON
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        Serial.println("解析JSON失败");
        return;
    }

    // 解析JSON
    // {"cmd": "enter_config"}
    const char *cmd = doc["cmd"];
    if (cmd)
    {
        Serial.printf("收到命令: %s\n", cmd);
        if (strcmp(cmd, "enter_ap_mode") == 0)
        {
#ifdef ENABLE_WIFI
            wifiManager.enterAPMode();
#endif
        }
        else if (strcmp(cmd, "exit_ap_mode") == 0)
        {
#ifdef ENABLE_WIFI
            wifiManager.exitAPMode();
#endif
        }
        else if (strcmp(cmd, "reset_wifi") == 0)
        {
#ifdef ENABLE_WIFI
            wifiManager.reset();
#endif
        }
        else if (strcmp(cmd, "set_sleep_time") == 0)
        {
            // {"cmd": "set_sleep_time", "sleep_time": 300}
            int sleepTime = doc["sleep_time"].as<int>();
            if (sleepTime > 0)
            {
                powerManager.setSleepTime(sleepTime);
            }
            else
            {
                Serial.println("休眠时间不能小于0");
            }
        }
        else if (strcmp(cmd, "gps_debug") == 0)
        {
            // {"cmd": "gps_debug", "enable": true}
            bool enable = doc["enable"].as<bool>();
            gpsManager.setDebug(enable);
            Serial.printf("GPS调试模式: %s\n", enable ? "开启" : "关闭");
        }
        // reboot
        else if (strcmp(cmd, "reboot") == 0)
        {
            Serial.println("重启设备");
            ESP.restart();
        }
#ifdef ENABLE_SDCARD
        // 格式化存储卡
        else if (strcmp(cmd, "format_sdcard") == 0)
        {
            Serial.println("格式化存储卡");
            if (!sdManager.handleSerialCommand("yes_format"))
            {
                Serial.println("格式化存储卡失败");
            }
            // if (!sdManager.handleSerialCommand("sd_repair"))
            // {
            //     Serial.println("修复存储卡失败");
            // }
            // if (!sdManager.handleSerialCommand("sd_format"))
            // {
            //     Serial.println("格式化存储卡失败");
            // }
        }
#endif
#ifdef ENABLE_AUDIO
        // 音频测试
        else if (strcmp(cmd, "audio_test") == 0)
        {
            Serial.println("执行音频测试");
            if (device_state.audioReady) {
                audioManager.testAudio();
            } else {
                Serial.println("音频系统未就绪");
            }
        }
        // 播放自定义音频
        else if (strcmp(cmd, "play_audio") == 0)
        {
            // {"cmd": "play_audio", "event": "boot_success"}
            const char* event = doc["event"];
            if (event && device_state.audioReady) {
                if (strcmp(event, "boot_success") == 0) {
                    audioManager.playBootSuccessSound();
                } else if (strcmp(event, "welcome") == 0) {
                    audioManager.playWelcomeVoice();
                } else if (strcmp(event, "wifi_connected") == 0) {
                    audioManager.playWiFiConnectedSound();
                } else if (strcmp(event, "gps_fixed") == 0) {
                    audioManager.playGPSFixedSound();
                } else if (strcmp(event, "low_battery") == 0) {
                    audioManager.playLowBatterySound();
                } else if (strcmp(event, "sleep_mode") == 0) {
                    audioManager.playSleepModeSound();
                } else if (strcmp(event, "custom") == 0) {
                    float frequency = doc["frequency"] | 1000.0;
                    int duration = doc["duration"] | 200;
                    audioManager.playCustomBeep(frequency, duration);
                }
                Serial.printf("播放音频事件: %s\n", event);
            } else {
                Serial.println("音频系统未就绪或事件参数无效");
            }
        }
#endif
    }
#endif
}

void mqttConnectionCallback(bool connected)
{
#ifndef DISABLE_MQTT
    if (connected)
    {
        Serial.println("MQTT连接成功，订阅主题");

        
        static bool firstConnect = true; // 添加静态变量标记首次连接
                                         // 配置主题
        String baseTopic = String("vehicle/v1/") + device_state.device_id;
        String telemetryTopic = baseTopic + "/telemetry/"; // telemetry: 遥测数据

        // 构建具体主题
        String deviceInfoTopic = telemetryTopic + "device";
        String gpsTopic = telemetryTopic + "location";
        String imuTopic = telemetryTopic + "motion";
        String controlTopic = baseTopic + "/ctrl/#"; // ctrl: 控制命令

        if (firstConnect)
        {
            firstConnect = false;

            mqttManager.addTopic("device_info", deviceInfoTopic.c_str(), 30000);

#ifdef ENABLE_IMU
            // mqttManager.addTopic("imu", imuTopic.c_str(), 1000);
#endif

            mqttManager.addTopic("gps", gpsTopic.c_str(), 1000);
            // 订阅主题 - 使用QoS=0
            mqttManager.subscribe(controlTopic.c_str(), 0);
        }
    }
    else
    {
        Serial.println("MQTT连接失败");
    }
#endif
}

Device device;

Device::Device()
{
    // 构造函数只做简单变量初始化
    device_state.sleep_time = 300;
    device_state.wifiConnected = false;
    device_state.bleConnected = false;
    device_state.battery_voltage = 0;
    device_state.battery_percentage = 0;
    device_state.is_charging = false;
    device_state.external_power = false;
    device_state.gpsReady = false;
    device_state.imuReady = false;
    device_state.compassReady = false;
    device_state.gsmReady = false;
    device_state.sdCardReady = false;
    device_state.sdCardSizeMB = 0;
    device_state.sdCardFreeMB = 0;
}

void Device::begin()
{
    // 从getVersionInfo()获取版本信息
    const VersionInfo &versionInfo = getVersionInfo();
    device_state.device_id = get_device_id();
    device_state.device_firmware_version = versionInfo.firmware_version;
    device_state.device_hardware_version = versionInfo.hardware_version;

    // 检查是否从深度睡眠唤醒
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    bool isWakeFromDeepSleep = (wakeup_reason != ESP_SLEEP_WAKEUP_UNDEFINED);

    // 打印启动信息
    if (isWakeFromDeepSleep)
    {
        Serial.println("[系统] 从深度睡眠唤醒，重新初始化系统...");
    }
    else
    {
        Serial.printf("[系统] 系统正常启动，硬件版本: %s, 固件版本: %s, 编译时间: %s\n", getVersionInfo().hardware_version,
                      getVersionInfo().firmware_version, getVersionInfo().build_time);
    }

#ifdef BAT_PIN
    // bat.setDebug(true);
    bat.begin();
#endif

#ifdef RTC_INT_PIN
    externalPower.setDebug(true);
    externalPower.begin();
#endif

    // LED初始化
#if defined(PWM_LED_PIN) || defined(LED_PIN)
    ledManager.begin();
#endif

#ifdef GPS_WAKE_PIN
    rtc_gpio_hold_dis((gpio_num_t)GPS_WAKE_PIN);
    Serial.println("[电源管理] GPS_WAKE_PIN 保持已解除");
#endif

#ifdef BLE_SERVER
    bs.begin();
#endif

#ifdef BLE_CLIENT
    bc.begin();
#endif

#ifdef ENABLE_COMPASS
    compass.begin();
#endif

#ifdef ENABLE_IMU
    imu.begin();
    // 如果是从深度睡眠唤醒，检查唤醒原因
    if (isWakeFromDeepSleep)
    {
        switch (wakeup_reason)
        {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("[系统] IMU运动唤醒检测到，记录运动事件");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("[系统] 定时器唤醒，检查系统状态");
            break;
        default:
            break;
        }
    }
#endif

#ifdef ENABLE_GSM
    ml307_at.setDebug(true);
    ml307_at.begin(115200);

#ifdef ENABLE_GNSS
    gpsManager.setGNSSEnabled(true);
#endif

#ifdef ENABLE_LBS
    gpsManager.setLBSEnabled(true);
#endif
#endif

#if (defined(ENABLE_WIFI) || defined(ENABLE_GSM)) && !defined(DISABLE_MQTT)
    // 创建 MQTT 配置
    MqttManagerConfig config;
    // 通用 MQTT 配置
    config.broker = MQTT_BROKER;
    config.port = MQTT_PORT;
    config.clientId = "";
    config.username = MQTT_USER;
    config.password = MQTT_PASSWORD;
    config.keepAlive = MQTT_KEEP_ALIVE;
    config.cleanSession = true; // 是否清除会话，true: 清除，false: 保留

    // 初始化 MQTT 管理器
    mqttManager.setDebug(true);
    ml307Mqtt.setDebug(true);
    if (!mqttManager.begin(config))
    {
        Serial.println("MQTT 初始化失败，将在运行时重试");
        return;
    }

    // 设置回调在开始之前
    mqttManager.onMessage(mqttMessageCallback);
    mqttManager.onConnect(mqttConnectionCallback);
    mqttManager.onMqttState([](MqttState state)
                            {
        switch (state) {
            case MqttState::CONNECTED:
#ifdef ENABLE_WIFI
                // WiFi模式下可以同时更新WiFi状态
                device_state.wifiConnected = true;
                Serial.println("MQTT连接成功");
#ifdef ENABLE_AUDIO
                // 播放WiFi连接成功音
                if (device_state.audioReady && AUDIO_WIFI_CONNECTED_ENABLED) {
                    audioManager.playWiFiConnectedSound();
                }
#endif
#else
                // GSM模式下只更新MQTT状态
                Serial.println("MQTT连接成功");
#endif
                ledManager.setLEDState(LED_BLINK_DUAL);
                break;
            case MqttState::DISCONNECTED:
#ifdef ENABLE_WIFI
                device_state.wifiConnected = false;
                Serial.println("MQTT连接断开");
#else
                Serial.println("MQTT连接断开");
#endif
                ledManager.setLEDState(LED_OFF);
                break;
            case MqttState::ERROR:
#ifdef ENABLE_WIFI
                device_state.wifiConnected = false;
                Serial.println("MQTT连接错误");
#else
                Serial.println("MQTT连接错误");
#endif
                ledManager.setLEDState(LED_BLINK_5_SECONDS);
                break;
        } });

    Serial.println("完成底层网络配置，wifi/gsm/mqtt 初始化完成");

#endif

    gpsManager.init();
    Serial.println("GPS初始化完成!");

#ifdef ENABLE_AUDIO
    // 音频系统初始化
    if (audioManager.begin()) {
        device_state.audioReady = true;
        Serial.println("音频系统初始化成功!");
        
        // 播放开机成功音
        if (AUDIO_BOOT_SUCCESS_ENABLED) {
            audioManager.playBootSuccessSound();
        }
    } else {
        device_state.audioReady = false;
        Serial.println("音频系统初始化失败!");
    }
#endif
}

// 通知特定状态变化
void notify_state_change(const char *state_name, const char *old_value, const char *new_value)
{
    Serial.printf("[状态变化] %s: %s -> %s\n", state_name, old_value, new_value);
}

// 更新设备状态并检查变化
void update_device_state()
{
    static device_state_t last_state;

    // 检查电池状态变化
    if (device_state.battery_percentage != last_state.battery_percentage)
    {
        notify_state_change("电池电量",
                            String(last_state.battery_percentage).c_str(),
                            String(device_state.battery_percentage).c_str());
        state_changes.battery_changed = true;
        
#ifdef ENABLE_AUDIO
        // 当电池电量降到20%以下时播放低电量警告音（避免频繁播放）
        if (device_state.battery_percentage <= 20 && 
            last_state.battery_percentage > 20 && 
            device_state.audioReady && 
            AUDIO_LOW_BATTERY_ENABLED) {
            audioManager.playLowBatterySound();
        }
#endif
    }

    // 检查外部电源状态变化
    if (device_state.external_power != last_state.external_power)
    {
        notify_state_change("外部电源",
                            last_state.external_power ? "已连接" : "未连接",
                            device_state.external_power ? "已连接" : "未连接");
        state_changes.external_power_changed = true;
    }

    // 检查网络状态变化 - 根据模式区分
#ifdef ENABLE_WIFI
    if (device_state.wifiConnected != last_state.wifiConnected)
    {
        notify_state_change("WiFi连接",
                            last_state.wifiConnected ? "已连接" : "未连接",
                            device_state.wifiConnected ? "已连接" : "未连接");
        state_changes.wifi_changed = true;
    }
#else
    if (device_state.gsmReady != last_state.gsmReady)
    {
        notify_state_change("4G连接",
                            last_state.gsmReady ? "已连接" : "未连接",
                            device_state.gsmReady ? "已连接" : "未连接");
        state_changes.gsm_changed = true;
    }
#endif

    // 检查BLE状态变化
    if (device_state.bleConnected != last_state.bleConnected)
    {
        notify_state_change("BLE连接",
                            last_state.bleConnected ? "已连接" : "未连接",
                            device_state.bleConnected ? "已连接" : "未连接");
        state_changes.ble_changed = true;
    }

    // 检查GPS状态变化 - 使用GPS管理器
    bool currentGpsReady = gpsManager.isReady();
    if (currentGpsReady != last_state.gpsReady)
    {
        notify_state_change("GPS状态",
                            last_state.gpsReady ? "就绪" : "未就绪",
                            currentGpsReady ? "就绪" : "未就绪");
        state_changes.gps_changed = true;
        device_state.gpsReady = currentGpsReady;
        
#ifdef ENABLE_AUDIO
        // 当GPS从未就绪变为就绪时播放定位成功音
        if (currentGpsReady && !last_state.gpsReady && device_state.audioReady && AUDIO_GPS_FIXED_ENABLED) {
            audioManager.playGPSFixedSound();
        }
#endif
    }

    // 检查LBS状态变化
#ifdef ENABLE_GSM
    // bool currentLbsReady = gpsManager.isLBSReady();
    // if (currentLbsReady != last_state.lbsReady)
    // {
    //     notify_state_change("LBS状态",
    //                         last_state.lbsReady ? "就绪" : "未就绪",
    //                         currentLbsReady ? "就绪" : "未就绪");
    //     device_state.lbsReady = currentLbsReady;
    // }

    // // 检查GNSS状态变化
    // bool currentGnssReady = gpsManager.isGNSSEnabled() && ml307.isGNSSReady();
    // if (currentGnssReady != last_state.gnssReady)
    // {
    //     notify_state_change("GNSS状态",
    //                         last_state.gnssReady ? "就绪" : "未就绪",
    //                         currentGnssReady ? "就绪" : "未就绪");
    //     device_state.gnssReady = currentGnssReady;
    // }
#endif

    // 检查IMU状态变化
    if (device_state.imuReady != last_state.imuReady)
    {
        notify_state_change("IMU状态",
                            last_state.imuReady ? "就绪" : "未就绪",
                            device_state.imuReady ? "就绪" : "未就绪");
        state_changes.imu_changed = true;
    }

    // 检查罗盘状态变化
    if (device_state.compassReady != last_state.compassReady)
    {
        notify_state_change("罗盘状态",
                            last_state.compassReady ? "就绪" : "未就绪",
                            device_state.compassReady ? "就绪" : "未就绪");
        state_changes.compass_changed = true;
    }

    // 检查休眠时间变化
    if (device_state.sleep_time != last_state.sleep_time)
    {
        notify_state_change("休眠时间",
                            String(last_state.sleep_time).c_str(),
                            String(device_state.sleep_time).c_str());
        state_changes.sleep_time_changed = true;
    }

    // 检查LED模式变化
    if (device_state.led_mode != last_state.led_mode)
    {
        notify_state_change("LED模式",
                            String(last_state.led_mode).c_str(),
                            String(device_state.led_mode).c_str());
        state_changes.led_mode_changed = true;
    }

    // 检查SD卡状态变化
    if (device_state.sdCardReady != last_state.sdCardReady)
    {
        notify_state_change("SD卡状态",
                            last_state.sdCardReady ? "就绪" : "未就绪",
                            device_state.sdCardReady ? "就绪" : "未就绪");
        state_changes.sdcard_changed = true;
    }

    // 检查音频状态变化
    if (device_state.audioReady != last_state.audioReady)
    {
        notify_state_change("音频状态",
                            last_state.audioReady ? "就绪" : "未就绪",
                            device_state.audioReady ? "就绪" : "未就绪");
        state_changes.audio_changed = true;
    }

    // 更新上一次状态
    last_state = device_state;

    // 重置状态变化标志
    state_changes = {0};
}

void device_loop()
{
    // Implementation of device_loop function
}
