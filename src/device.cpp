#include "device.h"

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
    Serial.printf("GPS Ready: %d\n", device_state.gpsReady);
    Serial.printf("IMU Ready: %d\n", device_state.imuReady);
    Serial.printf("Compass Ready: %d\n", device_state.compassReady);
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
// fw: 固件版本, hw: 硬件版本, wifi/ble/gps/imu/compass: 各模块状态, bat_v: 电池电压, bat_pct: 电池百分比
String device_state_to_json(device_state_t *state)
{
    StaticJsonDocument<256> doc; // 精简后更小即可
    doc["fw"] = device_state.device_firmware_version;
    doc["hw"] = device_state.device_hardware_version;
    doc["wifi"] = device_state.wifiConnected;
    doc["ble"] = device_state.bleConnected;
    doc["gps"] = device_state.gpsReady;
    doc["imu"] = device_state.imuReady;
    doc["compass"] = device_state.compassReady;
    doc["bat_v"] = device_state.battery_voltage;
    doc["bat_pct"] = device_state.battery_percentage;
    return doc.as<String>();
}

void mqttMessageCallback(const String &topic, const String &payload)
{
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
        if (strcmp(cmd, "enter_config") == 0)
        {
#ifdef ENABLE_WIFI
            wifiManager.enterConfigMode();
#endif
        }
        else if (strcmp(cmd, "exit_config") == 0)
        {
#ifdef ENABLE_WIFI
            wifiManager.exitConfigMode();
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
    }
}

void mqttConnectionCallback(bool connected)
{
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
        String controlTopic = baseTopic + "/control/#"; // control: 控制命令

        if (firstConnect)
        {
            firstConnect = false;

            mqttManager.addTopic("device_info", deviceInfoTopic.c_str(), 5000);

#ifdef ENABLE_IMU
            mqttManager.addTopic("imu", imuTopic.c_str(), 1000);
#endif
#ifdef ENABLE_GPS
            mqttManager.addTopic("gps", gpsTopic.c_str(), 1000);
#endif
            // 订阅主题 每次都要
            mqttManager.subscribe(controlTopic.c_str(), 1);
        }
    }else{
        Serial.println("MQTT连接失败");
    }
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
    device_state.gpsReady = false;
    device_state.imuReady = false;
    device_state.compassReady = false;
    device_state.gsmReady = false;
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
    bat.begin();
#endif

    // LED初始化
#if defined(PWM_LED_PIN) || defined(LED_PIN)
    ledManager.begin();
#endif

#ifdef GPS_WAKE_PIN
    rtc_gpio_hold_dis((gpio_num_t)GPS_WAKE_PIN);
    Serial.println("[电源管理] GPS_WAKE_PIN 保持已解除");
#endif

#ifdef ENABLE_GPS
    gps.begin();
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

    // IMU初始化
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

#if defined(ENABLE_WIFI) || defined(ENABLE_GSM)
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

#ifdef ENABLE_GSM
    ml307.setDebug(true);
    ml307Mqtt.setDebug(true);
    ml307.begin(921600);
#endif
    // 设置回调在开始之前
    mqttManager.onMessage(mqttMessageCallback);
    mqttManager.onConnect(mqttConnectionCallback);
    mqttManager.onNetworkState([](NetworkState state)
                               {
        switch (state) {
            case NetworkState::CONNECTED:
#ifdef ENABLE_WIFI
                device_state.wifiConnected = true;
                Serial.println("WiFi连接成功");
#else
                device_state.gsmReady = true;
                Serial.println("4G连接成功");
#endif
                ledManager.setLEDState(LEDManager::BLINK_DUAL); // 双闪
                break;
            case NetworkState::DISCONNECTED:
#ifdef ENABLE_WIFI
                device_state.wifiConnected = false;
                Serial.println("WiFi连接断开");
#else
                device_state.gsmReady = false;
                Serial.println("4G连接断开");
#endif
                ledManager.setLEDState(LEDManager::OFF); // 关闭
                break;
            case NetworkState::ERROR:
#ifdef ENABLE_WIFI
                device_state.wifiConnected = false;
                Serial.println("WiFi连接错误");
#else
                device_state.gsmReady = false;
                Serial.println("4G连接错误");
#endif
                ledManager.setLEDState(LEDManager::BLINK_5_SECONDS); // 快闪
                break;
        } });

    // mqttManager.setDebug(true);
    // 初始化 MQTT 管理器
    if (!mqttManager.begin(config))
    {
        Serial.println("MQTT 初始化失败，将在运行时重试");
        return;
    }
    Serial.println("完成底层网络配置，wifi/gsm/mqtt 初始化完成");

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

    // 检查GPS状态变化
    if (device_state.gpsReady != last_state.gpsReady)
    {
        notify_state_change("GPS状态",
                            last_state.gpsReady ? "就绪" : "未就绪",
                            device_state.gpsReady ? "就绪" : "未就绪");
        state_changes.gps_changed = true;
    }

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

    // 更新上一次状态
    last_state = device_state;

    // 重置状态变化标志
    state_changes = {0};
}
