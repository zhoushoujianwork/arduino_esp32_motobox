/*
 * GPS数据结构实现
 */

#include "GPSData.h"
#include <ArduinoJson.h>

// 全局GPS数据实例
gps_data_t gps_data = {0};

void reset_gps_data(gps_data_t& data) {
    memset(&data, 0, sizeof(gps_data_t));
}

bool is_gps_data_valid(const gps_data_t& data) {
    return data.latitude != 0.0 && 
           data.longitude != 0.0 && 
           data.satellites > 3;
}

String gps_data_to_json(const gps_data_t& data) {
    // 使用ArduinoJson库将gps_data转换为JSON字符串
    StaticJsonDocument<256> doc;
    doc["lat"] = data.latitude;
    doc["lon"] = data.longitude;
    doc["alt"] = data.altitude;
    doc["speed"] = data.speed;
    doc["satellites"] = data.satellites;
    doc["heading"] = data.heading;
    doc["year"] = data.year;
    doc["month"] = data.month;
    doc["day"] = data.day;
    doc["hour"] = data.hour;
    doc["minute"] = data.minute;
    doc["second"] = data.second;
    doc["hdop"] = data.hdop;

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
} 

void print_gps_data(const gps_data_t& data) {
    if (data.altitude > 3) {
        Serial.printf("[GPS] 位置: %.6f, %.6f, 速度: %.1f km/h, 精度: %.1f m, 海拔: %.1f m\n", 
                     data.latitude, data.longitude, data.speed, data.hdop, data.altitude);
    }else{
        Serial.printf("[GPS] 定位中... 可能来自LBS的定位 位置: %.8f, %.8f\n", data.latitude, data.longitude);
    }
}