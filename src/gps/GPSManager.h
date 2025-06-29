/*
 * GPS管理器 - 统一管理不同类型的GPS和LBS定位
 */

#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H

#include "GPSInterface.h"
#include "ExternalGPSAdapter.h"
#include "GPSData.h"
#include "LBSData.h"
#include "config.h"

class GPSManager {
public:
    static GPSManager& getInstance();
    
    void init();
    void loop();
    bool isReady();
    
    void setDebug(bool debug);
    String getType() const;
    
    // 新增控制方法
    void setGNSSEnabled(bool enable);
    void setLBSEnabled(bool enable);
    bool isGNSSEnabled() const;
    
    // 禁止拷贝和赋值
    GPSManager(const GPSManager&) = delete;
    GPSManager& operator=(const GPSManager&) = delete;

private:
    GPSManager() = default;
    ~GPSManager() = default;
    bool _debug = false;
    
    GPSInterface* _gpsInterface;
    bool _initialized;
    lbs_data_t _lbsData;
    unsigned long _lastLBSUpdate;
    static const unsigned long LBS_UPDATE_INTERVAL = 10000; // 1秒
    static const unsigned long LBS_REQUEST_INTERVAL = 1000; // 1秒
    void handleLBSUpdate();
    void debugPrint(const String &message);
};

// 全局GPS管理器实例
extern GPSManager& gpsManager;

#endif // GPS_MANAGER_H 