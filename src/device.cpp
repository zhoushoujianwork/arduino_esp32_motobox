#include "device.h"

extern const VersionInfo &getVersionInfo();

device_state_t device_state;

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
    const char *command = doc["command"];
    if (command)
    {
        Serial.printf("收到命令: %s\n", command);
    }
}

void mqttConnectionCallback(bool connected)
{
    Serial.printf("MQTT %s\n", connected ? "已连接" : "已断开");

    // 连接成功后自动订阅主题
    if (connected)
    {
        mqttManager.subscribe("test/topic");
    }
}

// WiFi 配置
const char *WIFI_SSID = "mikas iPhone";
const char *WIFI_PASSWORD = "11111111";

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

    // LED初始化
#ifdef PWM_LED_PIN
    pwmLed.begin();
    pwmLed.setMode(PWMLED::OFF); // 启动时设置为彩虹模式
#endif

#ifdef LED_PIN
    led.begin();
    led.setMode(isConnected ? LED::BLINK_DUAL : LED::BLINK_5_SECONDS);
#endif

    delay(200);

#ifdef GPS_WAKE_PIN
    rtc_gpio_hold_dis((gpio_num_t)GPS_WAKE_PIN);
    Serial.println("[电源管理] GPS_WAKE_PIN 保持已解除");
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
    config.cleanSession = true;

#ifdef ENABLE_WIFI// WiFi 模式
    config.wifiSsid = WIFI_SSID;
    config.wifiPassword = WIFI_PASSWORD;
#else
    ml307.setDebug(true);
    ml307.begin(921600);
#endif

    mqttManager.setDebug(true);
    // 初始化 MQTT 管理器
    if (!mqttManager.begin(config))
    {
        Serial.println("MQTT 初始化失败，将在运行时重试");
        return;
    }

    // 连接MQTT
    while (!mqttManager.connect()) {
        Serial.println("MQTT 连接失败，将在运行时重试");
        delay(1000);
    }


    // 设置回调
    mqttManager.onMessage(mqttMessageCallback);
    mqttManager.onConnect(mqttConnectionCallback);
    mqttManager.onNetworkState([](NetworkState state)
                               {
        switch (state) {
            case NetworkState::CONNECTED:
                Serial.println("网络已连接");
                break;
            case NetworkState::DISCONNECTED:
                Serial.println("网络已断开");
                break;
            case NetworkState::ERROR:
                Serial.println("网络连接错误");
                break;
        } });

    // 配置主题
    String baseTopic = String("vehicle/v1/") + device_state.device_id;
    String telemetryTopic = baseTopic + "/telemetry/"; // telemetry: 遥测数据

    // 构建具体主题
    String deviceInfoTopic = telemetryTopic + "device";
    String gpsTopic = telemetryTopic + "location";
    String imuTopic = telemetryTopic + "motion";
    String controlTopic = baseTopic + "/control/#"; // control: 控制命令

    mqttManager.addTopic("device_info", deviceInfoTopic.c_str(), 5000);
    mqttManager.addTopic("imu", imuTopic.c_str(), 1000);
    mqttManager.addTopic("gps", gpsTopic.c_str(), 1000);

    // 订阅主题
    mqttManager.subscribe(controlTopic.c_str(), 1);
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
}
