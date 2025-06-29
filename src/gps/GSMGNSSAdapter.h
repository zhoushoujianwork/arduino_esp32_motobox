/*
 * GSM模块GNSS功能适配器
 */

#ifndef GSM_GNSS_ADAPTER_H
#define GSM_GNSS_ADAPTER_H

#include "GPSInterface.h"
#include "../net/Ml307AtModem.h"

class GSMGNSSAdapter : public GPSInterface {
public:
    GSMGNSSAdapter(Ml307AtModem& modem);
    
    void begin() override;
    void loop() override;
    bool isReady() override;
    void setDebug(bool debug) override;
    String getType() const override { return "GSM_GNSS"; }
    
private:
    Ml307AtModem& _modem;
    bool _ready;
    void updateGNSSData();
    gps_data_t convertFromGNSSData(const gps_data_t& gnssData);
};

#endif // GSM_GNSS_ADAPTER_H 