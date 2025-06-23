/*
 * 外部GPS模块适配器实现
 */

#include "ExternalGPSAdapter.h"
#include "GPSData.h"

ExternalGPSAdapter::ExternalGPSAdapter(GPS& gps) : _gps(gps) {
}

void ExternalGPSAdapter::begin() {
    _gps.begin();
}

void ExternalGPSAdapter::loop() {
    _gps.loop();
    updateGPSData();
}

bool ExternalGPSAdapter::isReady() {
    return gps_data.satellites > 3 && gps_data.valid;
}

gps_data_t ExternalGPSAdapter::getData() {
    return gps_data;
}

void ExternalGPSAdapter::setDebug(bool debug) {
    _gps.setDebug(debug);
}

void ExternalGPSAdapter::updateGPSData() {
    // 更新全局gps_data结构
    gps_data.gpsType = "External";
    gps_data.valid = (gps_data.satellites > 3);
} 