/*
 * Air780EG GNSS适配器 - 实现GPSInterface接口
 * 让GPSManager能够统一管理Air780EG的GNSS功能
 */

#ifndef AIR780EG_GNSS_ADAPTER_H
#define AIR780EG_GNSS_ADAPTER_H

#include "GPSInterface.h"
#include "GPSData.h"
#include "config.h"

#ifdef USE_AIR780EG_GSM
#include "../net/Air780EGModem.h"

class Air780EGGNSSAdapter : public GPSInterface {
public:
    Air780EGGNSSAdapter(Air780EGModem& modem);
    virtual ~Air780EGGNSSAdapter() = default;
    
    // GPSInterface 接口实现
    virtual void begin() override;
    virtual void loop() override;
    virtual bool isReady() override;
    virtual void setDebug(bool debug) override;
    virtual String getType() const override;
    
    // Air780EG特有功能
    bool enableGNSS(bool enable);
    bool isGNSSEnabled() const;
    bool isGNSSFixed() const;
    int getSatelliteCount() const;
    float getHDOP() const;
    bool setUpdateRate(int hz);
    String getGNSSInfo() const;
    String getRawData() const;
    
    // 数据获取
    gps_data_t getGPSData() const;
    bool isGPSDataValid() const;

private:
    Air780EGModem& _modem;
    bool _debug;
    bool _initialized;
    bool _gnssEnabled;
    
    // 更新控制
    unsigned long _lastUpdate;
    static const unsigned long UPDATE_INTERVAL = 1000; // 1秒更新间隔
    
    // 数据缓存
    gps_data_t _gpsData;
    
    // 私有方法
    void debugPrint(const String& message);
    void updateGPSData();
    gps_data_t convertGNSSToGPS(const GNSSData& gnssData);
};

#endif // USE_AIR780EG_GSM

#endif // AIR780EG_GNSS_ADAPTER_H
