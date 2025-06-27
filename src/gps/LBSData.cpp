#include "LBSData.h"

lbs_data_t lbs_data = {0};

void reset_lbs_data(lbs_data_t& data) {
    memset(&data, 0, sizeof(lbs_data_t));
    data.state = LBSState::IDLE;
    data.valid = false;
}

bool is_lbs_data_valid(const lbs_data_t& data) {
    return data.valid && 
           data.longitude >= -180 && data.longitude <= 180 &&
           data.latitude >= -90 && data.latitude <= 90;
}

/**
 * 打印LBS数据用于调试
 */
void print_lbs_data(const lbs_data_t& data) {
    if (data.valid) {
        Serial.printf("[LBS] 位置: %.6f, %.6f, 半径: %d m, 状态: %d\n", 
                     data.latitude, data.longitude, data.radius, data.stat);
        if (data.descr.length() > 0) {
            Serial.printf("[LBS] 描述: %s\n", data.descr.c_str());
        }
    } else {
        Serial.println("[LBS] 数据无效");
    }
}

/**
 * 将LBS数据转换为GPS数据格式 - 通用函数
 */
gps_data_t convert_lbs_to_gps(const lbs_data_t& lbsData) {
    gps_data_t gpsData;
    memset(&gpsData, 0, sizeof(gps_data_t));
    
    if (is_lbs_data_valid(lbsData)) {
        gpsData.latitude = lbsData.latitude;
        gpsData.longitude = lbsData.longitude;
        // LBS无以下信息，全部置为无效/0
        gpsData.altitude = 0;
        gpsData.speed = 0;
        gpsData.heading = 0;
        gpsData.satellites = 0;
        gpsData.year = 0;
        gpsData.month = 0;
        gpsData.day = 0;
        gpsData.hour = 0;
        gpsData.minute = 0;
        gpsData.second = 0;
        gpsData.centisecond = 0;
        gpsData.hdop = 0;
        gpsData.gpsHz = 0;
    }
    
    return gpsData;
}