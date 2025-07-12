#include "device.h"
#include "utils/DebugUtils.h"
#include "config.h"
#include "tft/TFT.h"
#include "imu/qmi8658.h"
// GSM模块包含
#ifdef USE_AIR780EG_GSM
#include "Air780EG.h"
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
    Serial.printf("ICharging: %d\n", device_state.is_charging);
    Serial.printf("EPower: %d\n", device_state.external_power);
    Serial.printf("GSM Ready: %d\n", device_state.gsmReady);
    Serial.printf("IMU Ready: %d\n", device_state.imuReady);
    Serial.printf("Compass Ready: %d\n", device_state.compassReady);
    Serial.printf("SD Card Ready: %d\n", device_state.sdCardReady);
    if (device_state.sdCardReady)
    {
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
    doc["gsm"] = device_state.gsmReady;
    doc["gnss"] = device_state.gnssReady;
    doc["imu"] = device_state.imuReady;
    doc["compass"] = device_state.compassReady;
    doc["bat_v"] = device_state.battery_voltage;
    doc["bat_pct"] = device_state.battery_percentage;
    doc["is_charging"] = device_state.is_charging;
    doc["ext_power"] = device_state.external_power;
    doc["sd"] = device_state.sdCardReady;
    if (device_state.sdCardReady)
    {
        doc["sd_size"] = device_state.sdCardSizeMB;
        doc["sd_free"] = device_state.sdCardFreeMB;
    }
    doc["audio"] = device_state.audioReady;
    return doc.as<String>();
}

// 添加包装函数
String getDeviceStatusJSON()
{
    return device_state_to_json(&device_state);
}

String getLocationJSON()
{
    // 如果 gnss 定位差，则走wifi 和 lbs 获取定位
    if (!air780eg.getGNSS().isDataValid())
    {
        // 每2分钟一次定位，因为这个定位比较耗时
        static unsigned long lastUpdateTime = 0;
        if (millis() - lastUpdateTime > 60000*2)
        {
            lastUpdateTime = millis();
            if (!air780eg.getGNSS().updateWIFILocation())
                air780eg.getGNSS().updateLBS();
        }
    }
    return air780eg.getGNSS().getLocationJSON();
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
#ifdef ENABLE_WIFI

        if (strcmp(cmd, "enter_ap_mode") == 0)
        {
            wifiManager.enterAPMode();
        }
        else if (strcmp(cmd, "exit_ap_mode") == 0)
        {
            wifiManager.exitAPMode();
        }
        else if (strcmp(cmd, "reset_wifi") == 0)
        {
            wifiManager.reset();
        }
#endif
        if (strcmp(cmd, "set_sleep_time") == 0)
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
        // reboot
        else if (strcmp(cmd, "reboot") == 0 || strcmp(cmd, "restart") == 0)
        {
            Serial.println("重启设备");
            ESP.restart();
        }
#ifdef ENABLE_SDCARD
        // 格式化存储卡
        if (strcmp(cmd, "format_sdcard") == 0)
        {
            Serial.println("格式化存储卡");
            if (!sdManager.handleSerialCommand("yes_format"))
            {
                Serial.println("格式化存储卡失败");
            }
        }
#endif
        Serial.println("✅ 命令处理完成");
    }
    else
    {
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
        // 订阅控制主题
        air780eg.getMQTT().subscribe("vehicle/v1/" + device_state.device_id + "/ctrl/#", 1);
    }
    else
    {
        Serial.println("MQTT连接失败");
        Serial.println("❌ MQTT连接断开，订阅功能不可用");
    }
#endif
}

Device device;

Device::Device()
{
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

    try
    {
        imu.begin();
        device_state.imuReady = true; // 设置IMU状态为就绪
        Serial.println("[IMU] ✅ IMU系统初始化成功，状态已设置为就绪");
    }
    catch (...)
    {
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
#endif
    Serial.println("GPS初始化已延迟到任务中!");

#ifdef ENABLE_AUDIO
    // 音频系统初始化
    Serial.println("[音频] 开始初始化音频系统...");
    Serial.printf("[音频] 引脚配置 - WS:%d, BCLK:%d, DATA:%d\n", IIS_S_WS_PIN, IIS_S_BCLK_PIN, IIS_S_DATA_PIN);

    if (audioManager.begin())
    {
        device_state.audioReady = true;
        Serial.println("[音频] ✅ 音频系统初始化成功!");

        // 播放开机成功音（只播放一次）
        static bool bootSoundPlayed = false;
        if (AUDIO_BOOT_SUCCESS_ENABLED && !bootSoundPlayed)
        {
            Serial.println("[音频] 播放开机成功音...");
            audioManager.playBootSuccessSound();
            bootSoundPlayed = true;
        }
    }
    else
    {
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

        if (device_state.battery_percentage <= 20)
        {
            ledManager.setLEDState(LED_ON, LED_COLOR_RED, 5);
        }
        else
        {
            ledManager.setLEDState(LED_BREATH, LED_COLOR_GREEN, 5);
        }

#ifdef ENABLE_AUDIO
        // 当电池电量降到20%以下时播放低电量警告音（避免频繁播放）
        if (device_state.battery_percentage <= 20 &&
            last_state.battery_percentage > 20)
        {
            if (device_state.audioReady){
            Serial.println("[音频] 播放低电量警告音");
            audioManager.playLowBatterySound();
            }
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
#endif

    // 检查BLE状态变化
    if (device_state.bleConnected != last_state.bleConnected)
    {
        notify_state_change("BLE连接",
                            last_state.bleConnected ? "已连接" : "未连接",
                            device_state.bleConnected ? "已连接" : "未连接");
        state_changes.ble_changed = true;
    }

#ifdef ENABLE_GSM
    if (device_state.gsmReady != last_state.gsmReady)
    {
        notify_state_change("GNSS状态",
                            last_state.gsmReady ? "就绪" : "未就绪",
                            device_state.gsmReady ? "就绪" : "未就绪");
        device_state.gsmReady = device_state.gsmReady;
    }
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

void Device::initializeGSM()
{
//================ GSM模块初始化开始 ================
#ifdef USE_AIR780EG_GSM
    Serial.println("[GSM] 初始化Air780EG模块...");
    Serial.printf("[GSM] 引脚配置 - RX:%d, TX:%d, EN:%d\n", GSM_RX_PIN, GSM_TX_PIN, GSM_EN);
    // 设置日志级别 (可选)
    Air780EG::setLogLevel(AIR780EG_LOG_INFO);
    while (!air780eg.begin(&Serial1, 115200, GSM_RX_PIN, GSM_TX_PIN, GSM_EN))
    {
        Serial.println("[GSM] ❌ Air780EG基础初始化失败");
        device_state.gsmReady = false;
        delay(1000);
    }
    Serial.println("[GSM] ✅ Air780EG基础初始化成功");
    device_state.gsmReady = true;
    air780eg.getGNSS().enableGNSS();

#ifdef DISABLE_MQTT
    Serial.println("MQTT功能已禁用");
#else
    initializeMQTT();
#endif

#endif
    //================ GSM模块初始化结束 ================
}

bool Device::initializeMQTT()
{

#if (defined(ENABLE_WIFI) || defined(ENABLE_GSM))
    Serial.println("🔄 开始MQTT初始化...");

#ifdef USE_AIR780EG_GSM

    // 配置MQTT连接参数
    Air780EGMQTTConfig config;
    config.server = MQTT_BROKER;
    config.port = MQTT_PORT;
    config.client_id = MQTT_CLIENT_ID_PREFIX + device_state.device_hardware_version + "_" + device_state.device_id;
    config.username = MQTT_USERNAME;
    config.password = MQTT_PASSWORD;
    config.keepalive = 60;
    config.clean_session = true;
    // 初始化MQTT模块
    if (!air780eg.getMQTT().begin(config))
    {
        Serial.println("Failed to initialize MQTT module!");
        return false;
    }

    // 设置消息回调函数
    air780eg.getMQTT().setMessageCallback(mqttMessageCallback);

    // 设置连接状态回调
    air780eg.getMQTT().setConnectionCallback(mqttConnectionCallback);

    // 添加定时任务
    air780eg.getMQTT().addScheduledTask("device_status", "vehicle/v1/" + device_state.device_id + "/telemetry/device", getDeviceStatusJSON, 30000, 0, false);
    air780eg.getMQTT().addScheduledTask("location", "vehicle/v1/" + device_state.device_id + "/telemetry/location", getLocationJSON, 1000, 0, false);
    // air780eg.getMQTT().addScheduledTask("system_stats", mqttTopics.getSystemStatusTopic(), getSystemStatsJSON, 60, 0, false);

    // // 连接到MQTT服务器
    // if (!air780eg.getMQTT().connect())
    //     Serial.println("Failed to start MQTT connection, will retry later!");

#elif defined(USE_ML307_GSM)
    Serial.println("连接方式: ML307 4G网络");
#elif defined(ENABLE_WIFI)
    Serial.println("连接方式: WiFi网络");
#else
    Serial.println("连接方式: 未定义");
    return false;
#endif

#endif
    return false;
}