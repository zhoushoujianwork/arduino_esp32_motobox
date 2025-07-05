/*
 * GPS管理器 - 统一管理不同类型的GPS和GNSS定位
 * 支持GPS和GNSS互斥配置，GNSS模式下支持ML307和Air780EG
 * 通过AT指令获取Air780EG的GNSS数据
 */

#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H

#include "GPSInterface.h"
#include "ExternalGPSAdapter.h"
#include "GPSData.h"
#include "LBSData.h"
#include "config.h"

#ifdef USE_AIR780EG_GSM
#include "Air780EGGNSSAdapter.h"
#endif

// 定位模式枚举
enum class LocationMode {
    NONE,           // 未启用定位
    GPS_ONLY,       // 仅GPS模式（外部GPS模块）
    GNSS_ONLY,      // 仅GNSS模式（GSM模块内置GNSS）
    GNSS_WITH_LBS   // GNSS+LBS混合模式
};

// GNSS模块类型枚举
enum class GNSSModuleType {
    NONE,
    ML307,
    AIR780EG
};

class GPSManager {
public:
    static GPSManager& getInstance();
    
    // 基础功能
    void init();
    void loop();
    bool isReady();
    void setDebug(bool debug);
    String getType() const;
    
    // 模式控制方法
    void setLocationMode(LocationMode mode);
    LocationMode getLocationMode() const { return _locationMode; }
    
    // GNSS控制方法（互斥配置）
    void setGNSSEnabled(bool enable);
    bool isGNSSEnabled() const { return _gnssEnabled; }
    
    // LBS控制方法（可与GNSS同时启用）
    void setLBSEnabled(bool enable);
    bool isLBSEnabled() const { return _lbsEnabled; }
    
    // GPS控制方法（与GNSS互斥）
    void setGPSEnabled(bool enable);
    bool isGPSEnabled() const { return _gpsEnabled; }
    
    // 模块类型获取
    GNSSModuleType getGNSSModuleType() const { return _gnssModuleType; }
    String getGNSSModuleTypeString() const;
    
    // 数据获取方法
    gps_data_t getGPSData() const;
    lbs_data_t getLBSData() const;
    bool isGPSDataValid() const;
    bool isLBSDataValid() const;
    
    // 状态查询
    bool isGNSSFixed() const;
    int getSatelliteCount() const;
    float getHDOP() const;
    
    // 配置方法
    bool setGNSSUpdateRate(int hz);  // 设置GNSS更新频率
    
    // 禁止拷贝和赋值
    GPSManager(const GPSManager&) = delete;
    GPSManager& operator=(const GPSManager&) = delete;

private:
    GPSManager() = default;
    ~GPSManager() = default;
    
    // 基础成员
    bool _debug = false;
    bool _initialized = false;
    LocationMode _locationMode = LocationMode::NONE;
    GNSSModuleType _gnssModuleType = GNSSModuleType::NONE;
    
    // 功能启用状态
    bool _gnssEnabled = false;
    bool _lbsEnabled = false;
    bool _gpsEnabled = false;
    
    // GPS接口（用于外部GPS模块）
    GPSInterface* _gpsInterface = nullptr;
    
#ifdef USE_AIR780EG_GSM
    // Air780EG GNSS适配器
    Air780EGGNSSAdapter* _air780egGNSSAdapter = nullptr;
#endif
    
    // 数据缓存
    gps_data_t _gpsData;
    lbs_data_t _lbsData;
    
    // 更新时间戳
    unsigned long _lastGNSSUpdate = 0;
    unsigned long _lastLBSUpdate = 0;
    unsigned long _lastGPSUpdate = 0;
    
    // 更新间隔配置
    static const unsigned long GNSS_UPDATE_INTERVAL = 1000;  // 1秒
    static const unsigned long LBS_UPDATE_INTERVAL = 10000;  // 10秒
    static const unsigned long GPS_UPDATE_INTERVAL = 1000;   // 1秒
    
    // 私有方法
    void detectGNSSModuleType();
    void initializeLocationMode();
    void handleGNSSUpdate();
    void handleLBSUpdate();
    void handleGPSUpdate();
    void updateLocationData();
    void debugPrint(const String &message);
    
    // 模式切换方法
    void switchToGPSMode();
    void switchToGNSSMode();
    void switchToGNSSWithLBSMode();
    void disableAllModes();
};

// 全局GPS管理器实例
extern GPSManager& gpsManager;

#endif // GPS_MANAGER_H 