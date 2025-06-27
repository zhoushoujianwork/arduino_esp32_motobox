/*
 * 外部GPS模块适配器实现
 */

#include "ExternalGPSAdapter.h"
#include "GPSData.h"

ExternalGPSAdapter::ExternalGPSAdapter(GPS& gps) : _gps(gps) {
}

void ExternalGPSAdapter::begin() {
    Serial.println("[ExternalGPSAdapter] 开始初始化");
    _gps.begin();
}

void ExternalGPSAdapter::loop() {
    _gps.loop();
}

bool ExternalGPSAdapter::isReady() {
    return gps_data.satellites > 3;
}


void ExternalGPSAdapter::setDebug(bool debug) {
    _gps.setDebug(debug);
}
