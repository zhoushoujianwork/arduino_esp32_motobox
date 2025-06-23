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
}

bool ExternalGPSAdapter::isReady() {
    return gps_data.satellites > 3;
}

gps_data_t ExternalGPSAdapter::getData() {
    return gps_data;
}

void ExternalGPSAdapter::setDebug(bool debug) {
    _gps.setDebug(debug);
}
