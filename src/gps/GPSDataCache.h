/*
 * GPS数据缓存管理器
 * 统一管理GPS数据的定时获取和缓存，避免频繁AT调用
 */

#ifndef GPS_DATA_CACHE_H
#define GPS_DATA_CACHE_H

#include "GPSData.h"
#include "config.h"

#ifdef USE_AIR780EG_GSM
#include <Air780EG.h>
#endif

class GPSDataCache {
public:
    static GPSDataCache& getInstance();
    
    // 基础功能
    void init();
    void loop();  // 每秒调用一次AT+CGNSINF
    
    // 数据获取（从缓存读取，不触发AT调用）
    gps_data_t getGPSData() const { return _cachedGPSData; }
    bool isDataValid() const { return _dataValid; }
    bool isGNSSFixed() const { return _gnssFixed; }
    int getSatelliteCount() const { return _satelliteCount; }
    float getHDOP() const { return _hdop; }
    
    // 状态查询
    unsigned long getLastUpdateTime() const { return _lastUpdateTime; }
    unsigned long getDataAge() const { return millis() - _lastUpdateTime; }
    bool isDataFresh(unsigned long maxAge = 5000) const { return getDataAge() < maxAge; }
    
    // 配置
    void setUpdateInterval(unsigned long interval) { _updateInterval = interval; }
    void setDebug(bool debug) { _debug = debug; }
    
    // 禁止拷贝和赋值
    GPSDataCache(const GPSDataCache&) = delete;
    GPSDataCache& operator=(const GPSDataCache&) = delete;

private:
    GPSDataCache() = default;
    ~GPSDataCache() = default;
    
    // 成员变量
    bool _initialized = false;
    bool _debug = false;
    
    // 缓存数据
    gps_data_t _cachedGPSData;
    bool _dataValid = false;
    bool _gnssFixed = false;
    int _satelliteCount = 0;
    float _hdop = 0.0f;
    
    // 时间控制
    unsigned long _lastUpdateTime = 0;
    unsigned long _updateInterval = 1000;  // 默认1秒更新一次
    
#ifdef USE_AIR780EG_GSM
    // Air780EG模块引用
    Air780EG* _air780eg = nullptr;
#endif
    
    // 私有方法
    void updateGPSDataFromModem();
    bool parseGNSSResponse(const String& response);
    void debugPrint(const String& message);
    void resetCachedData();
};

// 全局实例
extern GPSDataCache& gpsDataCache;

#endif // GPS_DATA_CACHE_H
