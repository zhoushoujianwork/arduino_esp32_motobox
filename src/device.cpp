#include "device.h"
#include "config.h"
#include "tft/TFT.h"
#include "gps/GPSManager.h"
#include "imu/qmi8658.h"

// GSM模块包含
#ifdef USE_AIR780EG_GSM
#include "net/Air780EGModem.h"
extern Air780EGModem air780eg_modem;
#elif defined(USE_ML307_GSM)
#include "net/Ml307Mqtt.h"
extern Ml307Mqtt ml307Mqtt;
#endif

#ifdef ENABLE_SDCARD
#include "SD/SDManager.h"
extern SDManager sdManager;
#endif

#ifdef ENABLE_AUDIO
#include "audio/AudioManager.h"
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
    Serial.println("=== MQTT消息回调触发 ===");
    Serial.printf("收到消息 [%s]: %s\n", topic.c_str(), payload.c_str());
    Serial.printf("主题长度: %d, 负载长度: %d\n", topic.length(), payload.length());

    // 解析JSON
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        Serial.println("❌ 解析JSON失败: " + String(error.c_str()));
        Serial.println("原始负载: " + payload);
        Serial.println("=== MQTT消息回调结束 (JSON解析失败) ===");
        return;
    }

    Serial.println("✅ JSON解析成功");
    
    // 解析JSON
    // {"cmd": "enter_config"}
    const char *cmd = doc["cmd"];
    if (cmd)
    {
        Serial.printf("收到命令: %s\n", cmd);
        Serial.println("开始执行命令处理...");
        
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
        // 设置欢迎语音类型
        else if (strcmp(cmd, "set_welcome_voice") == 0)
        {
            // {\"cmd\": \"set_welcome_voice\", \"type\": 0}  // 0=默认, 1=力帆摩托
            int voiceType = doc["type"] | 0;
            device.setWelcomeVoiceType(voiceType);
            Serial.printf("设置欢迎语音类型: %d\n", voiceType);
        }
        // 播放欢迎语音
        else if (strcmp(cmd, "play_welcome_voice") == 0)
        {
            device.playWelcomeVoice();
        }
        // 获取欢迎语音信息
        else if (strcmp(cmd, "get_welcome_voice_info") == 0)
        {
            String info = device.getWelcomeVoiceInfo();
            Serial.println(info);
        }
        else
        {
            Serial.println("⚠️ 未知命令: " + String(cmd));
        }
        Serial.println("✅ 命令处理完成");
#endif
    } else {
        Serial.println("❌ 消息中未找到cmd字段");
    }
    Serial.println("=== MQTT消息回调结束 ===");
#endif
}

void mqttConnectionCallback(bool connected)
{
#ifndef DISABLE_MQTT
    Serial.printf("MQTT连接状态: %s\n", connected ? "已连接" : "断开");
    
    if (connected)
    {
        MQTT_DEBUG_PRINTLN("MQTT连接成功，开始配置订阅主题");

        static bool firstConnect = true; // 添加静态变量标记首次连接
        Serial.printf("首次连接标志: %s\n", firstConnect ? "是" : "否");
        
        // 配置主题
        String baseTopic = String("vehicle/v1/") + device_state.device_id;
        String telemetryTopic = baseTopic + "/telemetry/"; // telemetry: 遥测数据
        Serial.println("基础主题: " + baseTopic);
        Serial.println("遥测主题: " + telemetryTopic);

        // 构建具体主题
        String deviceInfoTopic = telemetryTopic + "device";
        String gpsTopic = telemetryTopic + "location";
        String imuTopic = telemetryTopic + "motion";
        String controlTopic = baseTopic + "/ctrl/#"; // ctrl: 控制命令
        
        Serial.println("设备信息主题: " + deviceInfoTopic);
        Serial.println("GPS主题: " + gpsTopic);
        Serial.println("IMU主题: " + imuTopic);
        Serial.println("控制命令主题: " + controlTopic);

        if (firstConnect)
        {
            Serial.println("执行首次连接初始化...");
            firstConnect = false;

            Serial.println("添加发布主题...");
            mqttManager.addTopic("device_info", deviceInfoTopic.c_str(), 30000);
            Serial.println("✅ 设备信息主题已添加");

#ifdef ENABLE_IMU
            // mqttManager.addTopic("imu", imuTopic.c_str(), 1000);
            Serial.println("IMU主题已跳过（被注释）");
#endif

            mqttManager.addTopic("gps", gpsTopic.c_str(), 1000);
            Serial.println("✅ GPS主题已添加");
            
            // 订阅主题 - 使用QoS=0
            Serial.println("开始订阅控制命令主题: " + controlTopic);
            bool subscribeResult = mqttManager.subscribe(controlTopic.c_str(), 0);
            Serial.printf("订阅结果: %s\n", subscribeResult ? "成功" : "失败");
            
            if (subscribeResult) {
                Serial.println("✅ MQTT订阅链路配置完成");
            } else {
                Serial.println("❌ MQTT订阅失败，消息接收可能不正常");
            }
        } else {
            Serial.println("非首次连接，跳过主题配置");
        }
    }
    else
    {
        MQTT_DEBUG_PRINTLN("MQTT连接失败");
        Serial.println("❌ MQTT连接断开，订阅功能不可用");
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
    Serial.println("[IMU] 开始初始化IMU系统...");
    Serial.printf("[IMU] 引脚配置 - SDA:%d, SCL:%d, INT:%d\n", IMU_SDA_PIN, IMU_SCL_PIN, IMU_INT_PIN);
    
    try {
        imu.begin();
        device_state.imuReady = true;  // 设置IMU状态为就绪
        Serial.println("[IMU] ✅ IMU系统初始化成功，状态已设置为就绪");
    } catch (...) {
        device_state.imuReady = false;
        Serial.println("[IMU] ❌ IMU系统初始化异常");
    }
    
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
#else
    device_state.imuReady = false;
    Serial.println("[IMU] IMU功能未启用 (ENABLE_IMU未定义)");
#endif

#ifdef ENABLE_GSM
#ifdef USE_AIR780EG_GSM
    // Air780EG模块初始化已在main.cpp中完成
    Serial.println("[Device] 使用Air780EG模块");
#elif defined(USE_ML307_GSM)
    // ML307模块初始化
    Serial.println("[Device] 使用ML307模块");
    ml307_at.setDebug(true);
    ml307_at.begin(115200);
#endif

    initializeGSM();

    // 先完成 mqtt 连接，再初始化其他功能
    initializeMQTT();


#endif
    gpsManager.init();
    Serial.println("GPS初始化完成!");

#ifdef ENABLE_AUDIO
    // 音频系统初始化
    Serial.println("[音频] 开始初始化音频系统...");
    Serial.printf("[音频] 引脚配置 - WS:%d, BCLK:%d, DATA:%d\n", IIS_S_WS_PIN, IIS_S_BCLK_PIN, IIS_S_DATA_PIN);
    
    if (audioManager.begin()) {
        device_state.audioReady = true;
        Serial.println("[音频] ✅ 音频系统初始化成功!");
        
        // 播放开机成功音（只播放一次）
        static bool bootSoundPlayed = false;
        if (AUDIO_BOOT_SUCCESS_ENABLED && !bootSoundPlayed) {
            Serial.println("[音频] 播放开机成功音...");
            audioManager.playBootSuccessSound();
            bootSoundPlayed = true;
        }
    } else {
        device_state.audioReady = false;
        Serial.println("[音频] ❌ 音频系统初始化失败!");
        Serial.println("[音频] 请检查:");
        Serial.println("[音频] 1. 音频引脚是否正确定义");
        Serial.println("[音频] 2. I2S硬件是否正常连接");
        Serial.println("[音频] 3. 引脚是否与其他功能冲突");
    }
#else
    device_state.audioReady = false;
    Serial.println("[音频] 音频功能未启用 (ENABLE_AUDIO未定义)");
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

void Device::initializeGSM() {
//================ GSM模块初始化开始 ================
#ifdef USE_AIR780EG_GSM
  Serial.println("step 6.5");
  Serial.println("[GSM] 初始化Air780EG模块...");

  Serial.printf("[GSM] 引脚配置 - RX:%d, TX:%d, EN:%d\n", GSM_RX_PIN, GSM_TX_PIN, GSM_EN);

  air780eg_modem.setDebug(true);
  if (air780eg_modem.begin())
  {
    Serial.println("[GSM] ✅ Air780EG基础初始化成功");
    device_state.gsmReady = true;

    // 检查GSM_EN引脚状态
    Serial.printf("[GSM] GSM_EN引脚状态: %s\n", digitalRead(GSM_EN) ? "HIGH" : "LOW");

    Serial.println("[GSM] 📡 网络注册和GNSS启用将在后台任务中完成");
  }
  else
  {
    Serial.println("[GSM] ❌ Air780EG基础初始化失败");
    device_state.gsmReady = false;

    // 调试信息
    Serial.printf("[GSM] GSM_EN引脚状态: %s\n", digitalRead(GSM_EN) ? "HIGH" : "LOW");
  }
#endif
  //================ GSM模块初始化结束 ================
}



bool Device::initializeMQTT() {
#if (defined(ENABLE_WIFI) || defined(ENABLE_GSM)) && !defined(DISABLE_MQTT)
    MQTT_DEBUG_PRINTLN("🔄 开始MQTT初始化...");
    
    // 创建 MQTT 配置
    MqttManagerConfig config;
    // 通用 MQTT 配置
    config.broker = MQTT_BROKER;
    config.port = MQTT_PORT;
    
    // 生成唯一的客户端ID，使用设备ID 
    // [系统] 系统正常启动，硬件版本: esp32-air780eg, 固件版本: v3.4.0+104, 编译时间: Jul  5 2025 15:14:31
    // 带上硬件版本+固件版本信息
    config.clientId = "ESP32_" + device_state.device_id + "_" + device_state.device_hardware_version + "_" + device_state.device_firmware_version;
    
    config.username = MQTT_USER;
    config.password = MQTT_PASSWORD;
    config.keepAlive = MQTT_KEEP_ALIVE;
    config.cleanSession = true; // 是否清除会话，true: 清除，false: 保留

    // 打印MQTT配置信息
    MQTT_DEBUG_PRINTLN("=== MQTT配置信息 ===");
    MQTT_DEBUG_PRINTF("MQTT服务器: %s\n", config.broker.c_str());
    MQTT_DEBUG_PRINTF("MQTT端口: %d\n", config.port);
    MQTT_DEBUG_PRINTF("MQTT客户端ID: %s\n", config.clientId.c_str());
    MQTT_DEBUG_PRINTF("MQTT用户名: %s\n", config.username.c_str());
    MQTT_DEBUG_PRINTF("MQTT密码: %s\n", config.password.length() > 0 ? "***已设置***" : "***未设置***");
    MQTT_DEBUG_PRINTF("保持连接: %d秒\n", config.keepAlive);
    MQTT_DEBUG_PRINTF("清除会话: %s\n", config.cleanSession ? "是" : "否");
    
#ifdef USE_AIR780EG_GSM
    MQTT_DEBUG_PRINTLN("连接方式: Air780EG 4G网络");
#elif defined(USE_ML307_GSM)
    MQTT_DEBUG_PRINTLN("连接方式: ML307 4G网络");
#elif defined(ENABLE_WIFI)
    MQTT_DEBUG_PRINTLN("连接方式: WiFi网络");
#else
    MQTT_DEBUG_PRINTLN("连接方式: 未定义");
#endif
    MQTT_DEBUG_PRINTLN("=== MQTT配置信息结束 ===");

    // 初始化 MQTT 管理器
    mqttManager.setDebug(MQTT_DEBUG_ENABLED);
    
    // 根据使用的GSM模块设置调试
#ifdef USE_AIR780EG_GSM
    air780eg_modem.setDebug(MQTT_DEBUG_ENABLED);
#elif defined(USE_ML307_GSM)
    ml307Mqtt.setDebug(MQTT_DEBUG_ENABLED);
#endif
    
    if (!mqttManager.begin(config))
    {
        MQTT_DEBUG_PRINTLN("❌ MQTT 初始化失败");
        return false;
    }
    
    MQTT_DEBUG_PRINTLN("✅ MQTT 管理器初始化成功");
    
    // 设置回调
    mqttManager.onMessage(mqttMessageCallback);
    mqttManager.onConnect(mqttConnectionCallback);
    mqttManager.onMqttState([](MqttState state)
                            {
        switch (state) {
            case MqttState::CONNECTED:
#ifdef ENABLE_WIFI
                // WiFi模式下可以同时更新WiFi状态
                device_state.wifiConnected = true;
                MQTT_DEBUG_PRINTLN("MQTT连接成功");
#ifdef ENABLE_AUDIO
                // 播放WiFi连接成功音
                if (device_state.audioReady && AUDIO_WIFI_CONNECTED_ENABLED) {
                    audioManager.playWiFiConnectedSound();
                }
#endif
#else
                // GSM模式下只更新MQTT状态
                MQTT_DEBUG_PRINTLN("MQTT连接成功");
#endif
                ledManager.setLEDState(LED_BLINK_DUAL);
                break;
            case MqttState::DISCONNECTED:
#ifdef ENABLE_WIFI
                device_state.wifiConnected = false;
                MQTT_DEBUG_PRINTLN("MQTT连接断开");
#else
                MQTT_DEBUG_PRINTLN("MQTT连接断开");
#endif
                ledManager.setLEDState(LED_OFF);
                break;
            case MqttState::ERROR:
#ifdef ENABLE_WIFI
                device_state.wifiConnected = false;
                MQTT_DEBUG_PRINTLN("MQTT连接错误");
#else
                MQTT_DEBUG_PRINTLN("MQTT连接错误");
#endif
                ledManager.setLEDState(LED_BLINK_5_SECONDS);
                break;
        } });

    // 等待MQTT连接成功
    MQTT_DEBUG_PRINTLN("🔄 等待MQTT连接成功...");
    unsigned long mqttConnectStart = millis();
    const unsigned long MQTT_CONNECT_TIMEOUT = 30000; // 30秒超时
    bool mqttConnected = false;
    
    while (!mqttConnected && (millis() - mqttConnectStart < MQTT_CONNECT_TIMEOUT)) {
        mqttManager.loop(); // 处理MQTT连接
        
        // 检查连接状态
        if (mqttManager.isConnected()) {
            mqttConnected = true;
            MQTT_DEBUG_PRINTLN("✅ MQTT连接成功！");
            break;
        }
        
        // 显示连接进度
        static unsigned long lastProgress = 0;
        if (millis() - lastProgress > 2000) {
            lastProgress = millis();
            unsigned long elapsed = millis() - mqttConnectStart;
            MQTT_DEBUG_PRINTF("⏳ MQTT连接中... (%lu/%lu秒)\n", elapsed/1000, MQTT_CONNECT_TIMEOUT/1000);
        }
        
        delay(100); // 短暂延时避免CPU占用过高
    }
    
    if (!mqttConnected) {
        MQTT_DEBUG_PRINTLN("⚠️ MQTT连接超时，将在运行时继续尝试");
        return false;
    }
    
    MQTT_DEBUG_PRINTLN("✅ MQTT初始化完成");
    return true;
    
#else
    MQTT_DEBUG_PRINTLN("⚠️ MQTT功能已禁用");
    return false;
#endif
}

// 欢迎语音配置方法
void Device::setWelcomeVoiceType(int voiceType) {
#ifdef ENABLE_AUDIO
    if (device_state.audioReady) {
        WelcomeVoiceType type = static_cast<WelcomeVoiceType>(voiceType);
        audioManager.setWelcomeVoiceType(type);
        Serial.printf("欢迎语音类型已设置为: %s\n", audioManager.getWelcomeVoiceDescription());
    } else {
        Serial.println("音频系统未就绪，无法设置欢迎语音类型");
    }
#else
    Serial.println("音频功能已禁用");
#endif
}

void Device::playWelcomeVoice() {
#ifdef ENABLE_AUDIO
    if (device_state.audioReady) {
        audioManager.playWelcomeVoice();
        Serial.printf("播放欢迎语音: %s\n", audioManager.getWelcomeVoiceDescription());
    } else {
        Serial.println("音频系统未就绪，无法播放欢迎语音");
    }
#else
    Serial.println("音频功能已禁用");
#endif
}

String Device::getWelcomeVoiceInfo() {
#ifdef ENABLE_AUDIO
    if (device_state.audioReady) {
        String info = "当前欢迎语音: ";
        info += audioManager.getWelcomeVoiceDescription();
        info += " (类型: ";
        info += String(static_cast<int>(audioManager.getWelcomeVoiceType()));
        info += ")";
        return info;
    } else {
        return "音频系统未就绪";
    }
#else
    return "音频功能已禁用";
#endif
}
