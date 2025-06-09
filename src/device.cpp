#include "device.h"
#include "net/NetManager.h"
#include "led/PWMLED.h"
#include "led/LED.h"
#include "wifi/WifiManager.h"
#include "ble/ble_server.h"
#include "qmi8658/IMU.h"
#include "power/PowerManager.h"
#include "ble/ble_client.h"
#include "compass/Compass.h"
#include "version.h"

#ifdef PWM_LED_PIN
extern PWMLED pwmLed;
#endif
#ifdef LED_PIN
extern LED led;
extern bool isConnected;
#endif
#ifdef MODE_ALLINONE
extern Compass compass;
#endif
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
extern BLES bs;
extern IMU imu;
#endif
#ifdef MODE_CLIENT
extern BLEC bc;
#endif
extern PowerManager powerManager;
extern NetManager netManager;
extern const VersionInfo &getVersionInfo();

Device::Device()
{
}

void Device::init()
{
    // 从getVersionInfo()获取版本信息
    const VersionInfo& versionInfo = getVersionInfo();
    device_state.device_firmware_version = versionInfo.firmware_version;
    device_state.device_hardware_version = versionInfo.hardware_version;
    
    device_state.sleep_time = 300; // 默认5分钟无活动进入低功耗模式
    device_state.wifiConnected = false;
#ifdef MODE_CLIENT
    device_state.bleConnected = false;
#else
    device_state.bleConnected = true; // 如果定义了MODE_CLIENT，则默认蓝牙连接 因为这个开关控制 TFT 的显示
#endif
    device_state.battery_voltage = 0;
    device_state.battery_percentage = 0;
    device_state.gpsReady = false;
    device_state.imuReady = false;
}

void Device::print_device_info()
{
    // 如果休眠倒计时的时候不打印
    if (powerManager.powerState == POWER_STATE_COUNTDOWN) {
        return;
    }
    Serial.println("Device Info:");
    Serial.printf("Device ID: %s\n", get_device_id().c_str());
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

void Device::set_wifi_connected(bool connected)
{
    if (device_state.wifiConnected != connected)
    {
        Serial.printf("WiFi状态变更: %d -> %d\n", device_state.wifiConnected, connected);
        device_state.wifiConnected = connected;
    }
}

void Device::set_ble_connected(bool connected)
{
    if (device_state.bleConnected != connected)
    {
        Serial.printf("BLE状态变更: %d -> %d\n", device_state.bleConnected, connected);
        device_state.bleConnected = connected;
    }
}

void Device::set_gps_ready(bool ready)
{
    if (device_state.gpsReady != ready)
    {
        Serial.printf("GPS状态变更: %d -> %d\n", device_state.gpsReady, ready);
        device_state.gpsReady = ready;
    }
}

void Device::set_imu_ready(bool ready)
{
    if (device_state.imuReady != ready)
    {
        Serial.printf("IMU状态变更: %d -> %d\n", device_state.imuReady, ready);
        device_state.imuReady = ready;
    }
}

device_state_t *Device::get_device_state()
{
    return &device_state;
}

void Device::set_device_state(device_state_t *state)
{
    device_state = *state;
}

// 生成精简版设备状态JSON
// fw: 固件版本, hw: 硬件版本, wifi/ble/gps/imu/compass: 各模块状态, bat_v: 电池电压, bat_pct: 电池百分比
String Device::device_state_to_json()
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

// 将gps_data_t结构体转换为JSON字符串
String Device::gps_data_to_json()
{
    // 使用ArduinoJson库将gps_data转换为JSON字符串
    StaticJsonDocument<256> doc;
    doc["lat"] = gps_data.latitude;
    doc["lon"] = gps_data.longitude;
    doc["alt"] = gps_data.altitude;
    doc["speed"] = gps_data.speed;
    doc["satellites"] = gps_data.satellites;
    doc["heading"] = gps_data.heading;
    doc["year"] = gps_data.year;
    doc["month"] = gps_data.month;
    doc["day"] = gps_data.day;
    doc["hour"] = gps_data.hour;
    doc["minute"] = gps_data.minute;
    doc["second"] = gps_data.second;
    doc["hdop"] = gps_data.hdop;

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

// 生成精简版IMU数据JSON
String Device::imu_data_to_json()
{
    StaticJsonDocument<256> doc;
    doc["ax"] = imu_data.accel_x; // X轴加速度
    doc["ay"] = imu_data.accel_y; // Y轴加速度
    doc["az"] = imu_data.accel_z; // Z轴加速度
    doc["gx"] = imu_data.gyro_x;  // X轴角速度
    doc["gy"] = imu_data.gyro_y;  // Y轴角速度
    doc["gz"] = imu_data.gyro_z;  // Z轴角速度
    // doc["mx"] = imu_data.mag_x;            // X轴磁场
    // doc["my"] = imu_data.mag_y;            // Y轴磁场
    // doc["mz"] = imu_data.mag_z;            // Z轴磁场
    doc["roll"] = imu_data.roll;        // 横滚角
    doc["pitch"] = imu_data.pitch;      // 俯仰角
    doc["yaw"] = imu_data.yaw;          // 航向角
    doc["temp"] = imu_data.temperature; // 温度
    return doc.as<String>();
}

void Device::printGpsData()
{
    char timeStr[30];
    sprintf(timeStr, "%04d-%02d-%02d %02d:%02d:%02d",
            gps_data.year, gps_data.month, gps_data.day,
            gps_data.hour, gps_data.minute, gps_data.second);

    Serial.println("gps_data: " + String(timeStr) + ", " +
                   String(gps_data.latitude) + ", " +
                   String(gps_data.longitude) + ", " +
                   String(gps_data.altitude) + ", " +
                   String(gps_data.speed) + ", " +
                   String(gps_data.heading) + ", " +
                   String(gps_data.satellites));

    // 添加HDOP状态显示
    const char *hdopStatus = "";
    if (gps_data.hdop == 0)
    {
        hdopStatus = "无数据";
    }
    else if (gps_data.hdop < 1.0)
    {
        hdopStatus = "优秀";
    }
    else if (gps_data.hdop < 2.0)
    {
        hdopStatus = "良好";
    }
    else if (gps_data.hdop < 5.0)
    {
        hdopStatus = "一般";
    }
    else
    {
        hdopStatus = "较差";
    }

    Serial.println("HDOP: " + String(gps_data.hdop, 1) + " (" + hdopStatus + ")");
}

void Device::printImuData()
{
    Serial.println("imu_data: " + String(imu_data.roll) + ", " + String(imu_data.pitch) + ", " + String(imu_data.yaw) + ", " + String(imu_data.temperature));
}

gps_data_t *Device::get_gps_data()
{
    return &gps_data;
}

void Device::set_gps_data(gps_data_t *data)
{
    gps_data = *data;
}

imu_data_t *Device::get_imu_data()
{
    return &imu_data;
}

void Device::set_imu_data(imu_data_t *data)
{
    imu_data = *data;
}

// getTotalDistanceKm
float Device::getTotalDistanceKm()
{
    // 获取当前GPS数据
    gps_data_t *currentGpsData = get_gps_data();

    // 如果没有有效的GPS数据或卫星数量不足，返回当前累积距离
    if (currentGpsData->satellites < 3)
    {
        return totalDistanceKm;
    }

    // 获取当前时间
    unsigned long currentTime = millis();

    // 如果是第一次计算，初始化lastDistanceTime
    if (lastDistanceTime == 0)
    {
        lastDistanceTime = currentTime;
        return totalDistanceKm;
    }

    // 计算时间间隔（秒）
    float timeInterval = (currentTime - lastDistanceTime) / 1000.0;

    // 计算距离增量
    float distanceIncrement = (currentGpsData->speed / 3600.0) * timeInterval;

    // 累加距离
    totalDistanceKm += distanceIncrement;

    // 更新最后计算时间
    lastDistanceTime = currentTime;

    return totalDistanceKm;
}

compass_data_t *Device::get_compass_data()
{
    return &compass_data;
}

void Device::set_compass_data(compass_data_t *data)
{
    compass_data = *data;
}

String Device::compass_data_to_json()
{
    StaticJsonDocument<200> doc;
    doc["x"] = compass_data.x;
    doc["y"] = compass_data.y;
    doc["z"] = compass_data.z;
    doc["heading"] = compass_data.heading;
    doc["direction"] = compass.getDirectionChar(compass_data.heading);
    String output;
    serializeJson(doc, output);
    return output;
}

void Device::printCompassData()
{
    Serial.printf("Compass: X=%.1f, Y=%.1f, Z=%.1f, Heading=%.1f, Direction=%s\n",
                  compass_data.x, compass_data.y, compass_data.z, compass_data.heading, compass.getDirectionChar(compass_data.heading));
}

void Device::initializeHardware()
{
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
    led.setMode(isConnected ? LED::BLINK_DUAL : LED::BLINK_5_SECONDS);
#endif

    // 初始化设备
    this->init();

#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    delay(200);
    netManager.begin();
    
    // 蓝牙服务器初始化
    bs.setup();

#if defined(GPS_COMPASS_SDA) && defined(GPS_COMPASS_SCL)
    // 显示屏初始化完成后，再初始化 GPS
    Serial.println("[系统] 初始化GPS...");
    Wire.begin(GPS_COMPASS_SDA, GPS_COMPASS_SCL);
    compass.begin();
    compass.setDeclination(-5.9f);
#endif

#ifdef GPS_WAKE_PIN
    rtc_gpio_hold_dis((gpio_num_t)GPS_WAKE_PIN);
    Serial.println("[电源管理] GPS_WAKE_PIN 保持已解除");
#endif

    gps.begin();
    

    // IMU初始化
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
            // 这里可以添加定期状态检查代码
            break;
        default:
            break;
        }
    }
#endif

    // 电源管理初始化
    powerManager.begin();

#ifdef MODE_CLIENT
    // 蓝牙客户端初始化
    bc.setup();
#endif
}


