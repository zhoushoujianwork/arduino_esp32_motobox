#include "device.h"

Device::Device()
{
}

void Device::init()
{
    device_state.version = VERSION;
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
    Serial.print("Device Info:");
    Serial.printf("WiFi Connected: %d\t", device_state.wifiConnected);
    Serial.printf("BLE Connected: %d\t", device_state.bleConnected);
    Serial.printf("Battery Voltage: %d\t", device_state.battery_voltage);
    Serial.printf("Battery Percentage: %d\t", device_state.battery_percentage);
    Serial.printf("GPS Ready: %d\t", device_state.gpsReady);
    Serial.printf("IMU Ready: %d\n", device_state.imuReady);
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
    if (device_state.wifiConnected != connected) {
        Serial.printf("WiFi状态变更: %d -> %d\n", device_state.wifiConnected, connected);
        device_state.wifiConnected = connected;
    }
}

void Device::set_ble_connected(bool connected)
{
    if (device_state.bleConnected != connected) {
        Serial.printf("BLE状态变更: %d -> %d\n", device_state.bleConnected, connected);
        device_state.bleConnected = connected;
    }
}

void Device::set_gps_ready(bool ready)
{
    if (device_state.gpsReady != ready) {
        Serial.printf("GPS状态变更: %d -> %d\n", device_state.gpsReady, ready);
        device_state.gpsReady = ready;
    }
}

void Device::set_imu_ready(bool ready)
{
    if (device_state.imuReady != ready) {
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

String Device::device_state_to_json()
{
    StaticJsonDocument<200> doc;
    doc["version"] = device_state.version;
    doc["wifi_connected"] = device_state.wifiConnected;
    doc["ble_connected"] = device_state.bleConnected;
    doc["gps_ready"] = device_state.gpsReady;
    doc["imu_ready"] = device_state.imuReady;
    doc["compass_ready"] = device_state.compassReady;
    doc["battery_voltage"] = device_state.battery_voltage;
    doc["battery_percentage"] = device_state.battery_percentage;
    return doc.as<String>();
}

// 将gps_data_t结构体转换为JSON字符串
String Device::gps_data_to_json()
{
    // 使用ArduinoJson库将gps_data转换为JSON字符串
    StaticJsonDocument<200> doc;
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
    
    // HDOP信息
    doc["hdop"] = gps_data.hdop;
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

// 将imu_data_t结构体转换为JSON字符串
String Device::imu_data_to_json()
{
    // 使用ArduinoJson库将imu_data转换为JSON字符串
    StaticJsonDocument<200> doc;
    doc["roll"] = imu_data.roll;
    doc["pitch"] = imu_data.pitch;
    doc["yaw"] = imu_data.yaw;
    doc["temperature"] = imu_data.temperature;
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
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
    const char* hdopStatus = "";
    if (gps_data.hdop == 0) {
        hdopStatus = "无数据";
    } else if(gps_data.hdop < 1.0) {
        hdopStatus = "优秀";
    } else if(gps_data.hdop < 2.0) {
        hdopStatus = "良好";
    } else if(gps_data.hdop < 5.0) {
        hdopStatus = "一般";
    } else {
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
float Device::getTotalDistanceKm() {
    // 获取当前GPS数据
    gps_data_t* currentGpsData = get_gps_data();
    
    // 如果没有有效的GPS数据或卫星数量不足，返回当前累积距离
    if (currentGpsData->satellites < 3) {
        return totalDistanceKm;
    }

    // 获取当前时间
    unsigned long currentTime = millis();

    // 如果是第一次计算，初始化lastDistanceTime
    if (lastDistanceTime == 0) {
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

compass_data_t *Device::get_compass_data() {
    return &compass_data;
}

void Device::set_compass_data(compass_data_t *data) {
    compass_data = *data;
}

String Device::compass_data_to_json() {
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

void Device::printCompassData() {
    Serial.printf("Compass: X=%.1f, Y=%.1f, Z=%.1f, Heading=%.1f, Direction=%s\n",
                 compass_data.x, compass_data.y, compass_data.z, compass_data.heading, compass.getDirectionChar(compass_data.heading));
}


