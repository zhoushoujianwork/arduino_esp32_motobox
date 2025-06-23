/*
 * 外部GPS模块适配器
 */

#ifndef EXTERNAL_GPS_ADAPTER_H
#define EXTERNAL_GPS_ADAPTER_H

#include "GPSInterface.h"
#include "GPS.h"

class ExternalGPSAdapter : public GPSInterface {
public:
    ExternalGPSAdapter(GPS& gps);
    
    void begin() override;
    void loop() override;
    bool isReady() override;
    gps_data_t getData() override;
    void setDebug(bool debug) override;
    String getType() const override { return "External"; }
    
private:
    GPS& _gps;
};

#endif // EXTERNAL_GPS_ADAPTER_H 