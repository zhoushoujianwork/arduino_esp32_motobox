/*
 * GSM模块GNSS功能适配器实现
 */

#include "GSMGNSSAdapter.h"
#include "GPSData.h"

GSMGNSSAdapter::GSMGNSSAdapter(Ml307AtModem& modem) 
    : _modem(modem), _ready(false) {
}

void GSMGNSSAdapter::begin() {
    if (_modem.gnssInit()) {
        _ready = true;
    }
}

void GSMGNSSAdapter::loop() {
    if (_ready) {
        updateGNSSData();
    }
}

bool GSMGNSSAdapter::isReady() {
    return _ready && _modem.isGNSSReady();
}

gps_data_t GSMGNSSAdapter::getData() {
    gps_data_t gnssData = _modem.getGNSSData();
    return convertFromGNSSData(gnssData);
}

void GSMGNSSAdapter::setDebug(bool debug) {
    _modem.setDebug(debug);
}

void GSMGNSSAdapter::updateGNSSData() {
    if (_modem.updateGNSSData()) {
        gps_data_t gnssData = _modem.getGNSSData();
        gps_data = convertFromGNSSData(gnssData);
    }
}

gps_data_t GSMGNSSAdapter::convertFromGNSSData(const gps_data_t& gnssData) {
    gps_data_t gpsData = {};
    reset_gps_data(gpsData);
    
    if (gnssData.valid) {
        gpsData.valid = true;
        gpsData.gpsType = "GSM_GNSS";
        
        // 时间信息
        gpsData.year = gnssData.year;
        gpsData.month = gnssData.month;
        gpsData.day = gnssData.day;
        gpsData.hour = gnssData.hour;
        gpsData.minute = gnssData.minute;
        gpsData.second = gnssData.second;
        
        // 位置信息
        gpsData.latitude = gnssData.latitude;
        gpsData.longitude = gnssData.longitude;
        gpsData.altitude = gnssData.altitude;
        
        // 精度信息
        gpsData.hdop = gnssData.hdop;
        
        // 运动信息
        gpsData.speed = gnssData.speed;
        gpsData.heading = gnssData.heading;
        
        // 卫星信息
        gpsData.satellites = gnssData.satellites;
    }
    
    return gpsData;
} 