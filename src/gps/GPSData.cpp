/*
 * GPS数据结构实现
 */

#include "GPSData.h"
#include <ArduinoJson.h>

// 全局GPS数据实例
gps_data_t gps_data = {0};

void reset_gps_data(gps_data_t& data) {
    memset(&data, 0, sizeof(gps_data_t));
    data.valid = false;
    data.gpsType = "";
}

bool is_gps_data_valid(const gps_data_t& data) {
    return data.valid && 
           data.latitude != 0.0 && 
           data.longitude != 0.0 && 
           data.satellites > 3;
}

String gps_data_to_json(const gps_data_t& data) {
    DynamicJsonDocument doc(1024);
    
    doc["valid"] = data.valid;
    doc["type"] = data.gpsType;
    
    // 时间信息
    JsonObject time = doc.createNestedObject("time");
    time["year"] = data.year;
    time["month"] = data.month;
    time["day"] = data.day;
    time["hour"] = data.hour;
    time["minute"] = data.minute;
    time["second"] = data.second;
    time["centisecond"] = data.centisecond;
    
    // 位置信息
    JsonObject position = doc.createNestedObject("position");
    position["latitude"] = data.latitude;
    position["longitude"] = data.longitude;
    position["altitude"] = data.altitude;
    
    // 精度信息
    JsonObject accuracy = doc.createNestedObject("accuracy");
    accuracy["hdop"] = data.hdop;
    accuracy["pdop"] = data.pdop;
    accuracy["vdop"] = data.vdop;
    accuracy["hpa"] = data.hpa;
    accuracy["vpa"] = data.vpa;
    
    // 运动信息
    JsonObject motion = doc.createNestedObject("motion");
    motion["speed"] = data.speed;
    motion["heading"] = data.heading;
    
    // 卫星信息
    JsonObject satellites = doc.createNestedObject("satellites");
    satellites["visible"] = data.satellites;
    satellites["used"] = data.satellites_used;
    satellites["cn0_max"] = data.cn0_max;
    
    // 配置信息
    doc["gpsHz"] = data.gpsHz;
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
} 