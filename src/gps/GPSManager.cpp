/*
 * GPS管理器实现 - 统一管理GPS、GNSS和LBS定位
 * 支持GPS和GNSS互斥配置，GNSS模式下支持ML307和Air780EG
 */

#include "GPSManager.h"

// GSM模块支持
#ifdef ENABLE_GSM
#include "GSMGNSSAdapter.h"
#ifdef USE_AIR780EG_GSM
#include "../net/Air780EGModem.h"
extern Air780EGModem air780eg_modem;
#elif defined(USE_ML307_GSM)
#include "../net/Ml307AtModem.h"
extern Ml307AtModem ml307_at;
#endif
#endif

// 外部GPS模块支持
#ifdef ENABLE_GPS
#include "ExternalGPSAdapter.h"
#include "GPS.h"
extern GPS gps;
#endif

GPSManager &GPSManager::getInstance()
{
    static GPSManager instance;
    return instance;
}

void GPSManager::init()
{
    _debug = true;
    Serial.println("[GPSManager] 开始初始化定位管理器");
    
    // 检测GNSS模块类型
    detectGNSSModuleType();
    
    // 初始化定位模式
    initializeLocationMode();
    
    // 初始化数据结构
    reset_gps_data(_gpsData);
    _lbsData.valid = false;
    _lastGNSSUpdate = 0;
    _lastLBSUpdate = 0;
    _lastGPSUpdate = 0;
    
    _initialized = true;
    debugPrint("定位管理器初始化完成");
}

/*
 * 主循环 - 根据当前定位模式更新相应的定位数据
 * GPS和GNSS是互斥的，LBS可以与GNSS同时使用作为辅助定位
 */
void GPSManager::loop()
{
    if (!_initialized) {
        return;
    }
    
    // 根据当前定位模式处理数据更新
    switch (_locationMode) {
        case LocationMode::GPS_ONLY:
            handleGPSUpdate();
            break;
            
        case LocationMode::GNSS_ONLY:
            handleGNSSUpdate();
            break;
            
        case LocationMode::GNSS_WITH_LBS:
            handleGNSSUpdate();
            handleLBSUpdate();
            break;
            
        case LocationMode::NONE:
        default:
            // 无定位模式，不处理
            break;
    }
    
    // 更新统一的定位数据
    updateLocationData();
}

bool GPSManager::isReady()
{
    switch (_locationMode) {
        case LocationMode::GPS_ONLY:
            return is_gps_data_valid(_gpsData) && _gpsData.altitude > 3;
            
        case LocationMode::GNSS_ONLY:
            return is_gps_data_valid(_gpsData) && _gpsData.altitude > 3;
            
        case LocationMode::GNSS_WITH_LBS:
            return (is_gps_data_valid(_gpsData) && _gpsData.altitude > 3) || _lbsData.valid;
            
        case LocationMode::NONE:
        default:
            return false;
    }
}

void GPSManager::setDebug(bool debug)
{
    _debug = debug;
    if (_gpsInterface) {
        _gpsInterface->setDebug(debug);
    }
}

String GPSManager::getType() const
{
    switch (_locationMode) {
        case LocationMode::GPS_ONLY:
            return "GPS";
            
        case LocationMode::GNSS_ONLY:
            return "GNSS-" + getGNSSModuleTypeString();
            
        case LocationMode::GNSS_WITH_LBS:
            return "GNSS+LBS-" + getGNSSModuleTypeString();
            
        case LocationMode::NONE:
        default:
            return "None";
    }
}

// 设置定位模式
void GPSManager::setLocationMode(LocationMode mode)
{
    if (_locationMode == mode) {
        return;  // 模式相同，无需切换
    }
    
    debugPrint("切换定位模式: " + String((int)_locationMode) + " -> " + String((int)mode));
    
    // 先禁用当前模式
    disableAllModes();
    
    // 设置新模式
    _locationMode = mode;
    
    // 启用新模式
    switch (mode) {
        case LocationMode::GPS_ONLY:
            switchToGPSMode();
            break;
            
        case LocationMode::GNSS_ONLY:
            switchToGNSSMode();
            break;
            
        case LocationMode::GNSS_WITH_LBS:
            switchToGNSSWithLBSMode();
            break;
            
        case LocationMode::NONE:
        default:
            // 已经禁用所有模式
            break;
    }
}

// GNSS控制方法
void GPSManager::setGNSSEnabled(bool enable)
{
    if (_gnssEnabled == enable) {
        return;
    }
    
    _gnssEnabled = enable;
    debugPrint(String("GNSS ") + (enable ? "启用" : "禁用"));
    
#ifdef USE_AIR780EG_GSM
    air780eg_modem.enableGNSS(enable);
#elif defined(USE_ML307_GSM)
    // ML307暂时不支持GNSS控制
    debugPrint("ML307暂不支持GNSS控制");
#endif
    
    // 如果禁用GNSS且当前是GNSS模式，切换到NONE模式
    if (!enable && (_locationMode == LocationMode::GNSS_ONLY || 
                    _locationMode == LocationMode::GNSS_WITH_LBS)) {
        if (_locationMode == LocationMode::GNSS_WITH_LBS && _lbsEnabled) {
            // 如果LBS还启用，保持LBS功能（但这需要重新设计架构）
            debugPrint("GNSS禁用，但LBS仍启用");
        } else {
            setLocationMode(LocationMode::NONE);
        }
    }
}

// LBS控制方法
void GPSManager::setLBSEnabled(bool enable)
{
    if (_lbsEnabled == enable) {
        return;
    }
    
    _lbsEnabled = enable;
    debugPrint(String("LBS ") + (enable ? "启用" : "禁用"));
    
#ifdef USE_AIR780EG_GSM
    air780eg_modem.enableLBS(enable);
#elif defined(USE_ML307_GSM)
    ml307_at.enableLBS(enable);
#endif
}

// GPS控制方法
void GPSManager::setGPSEnabled(bool enable)
{
    if (_gpsEnabled == enable) {
        return;
    }
    
    _gpsEnabled = enable;
    debugPrint(String("GPS ") + (enable ? "启用" : "禁用"));
    
    if (enable) {
        // 启用GPS时，禁用GNSS（互斥）
        if (_gnssEnabled) {
            setGNSSEnabled(false);
        }
        setLocationMode(LocationMode::GPS_ONLY);
    } else {
        // 禁用GPS
        if (_locationMode == LocationMode::GPS_ONLY) {
            setLocationMode(LocationMode::NONE);
        }
    }
}

// 获取GNSS模块类型字符串
String GPSManager::getGNSSModuleTypeString() const
{
    switch (_gnssModuleType) {
        case GNSSModuleType::AIR780EG:
            return "Air780EG";
        case GNSSModuleType::ML307:
            return "ML307";
        case GNSSModuleType::NONE:
        default:
            return "None";
    }
}

// 数据获取方法
gps_data_t GPSManager::getGPSData() const
{
    return _gpsData;
}

lbs_data_t GPSManager::getLBSData() const
{
    return _lbsData;
}

bool GPSManager::isGPSDataValid() const
{
    return is_gps_data_valid(_gpsData);
}

bool GPSManager::isLBSDataValid() const
{
    return _lbsData.valid;
}

// 状态查询方法
bool GPSManager::isGNSSFixed() const
{
    return is_gps_data_valid(_gpsData) && _gpsData.altitude > 3;
}

int GPSManager::getSatelliteCount() const
{
    return _gpsData.satellites;
}

float GPSManager::getHDOP() const
{
    return _gpsData.hdop;
}

// 配置方法
bool GPSManager::setGNSSUpdateRate(int hz)
{
    if (hz <= 0 || hz > 10) {
        debugPrint("无效的GNSS更新频率: " + String(hz));
        return false;
    }
    
#ifdef USE_AIR780EG_GSM
    return air780eg_modem.setGNSSUpdateRate(hz);
#elif defined(USE_ML307_GSM)
    // ML307暂不支持设置GNSS更新频率
    debugPrint("ML307暂不支持设置GNSS更新频率");
    return false;
#else
    return false;
#endif
}

// 私有方法实现

void GPSManager::debugPrint(const String &message)
{
    if (_debug)
    {
        Serial.println("[GPSManager] [debug] " + message);
    }
}

// 检测GNSS模块类型
void GPSManager::detectGNSSModuleType()
{
#ifdef USE_AIR780EG_GSM
    _gnssModuleType = GNSSModuleType::AIR780EG;
    debugPrint("检测到Air780EG GNSS模块");
#elif defined(USE_ML307_GSM)
    _gnssModuleType = GNSSModuleType::ML307;
    debugPrint("检测到ML307 GNSS模块");
#else
    _gnssModuleType = GNSSModuleType::NONE;
    debugPrint("未检测到GNSS模块");
#endif
}

// 初始化定位模式
void GPSManager::initializeLocationMode()
{
#ifdef ENABLE_GPS
    // 外部GPS模式
    debugPrint("配置为外部GPS模式");
    _locationMode = LocationMode::GPS_ONLY;
    _gpsEnabled = true;
    _gpsInterface = new ExternalGPSAdapter(gps);
    if (_gpsInterface) {
        _gpsInterface->begin();
        debugPrint("外部GPS初始化完成");
    }
#elif defined(ENABLE_GSM)
    // GSM模块GNSS模式
    #ifdef ENABLE_GNSS
        #ifdef ENABLE_LBS
            debugPrint("配置为GNSS+LBS混合模式");
            _locationMode = LocationMode::GNSS_WITH_LBS;
            _gnssEnabled = true;
            _lbsEnabled = true;
        #else
            debugPrint("配置为纯GNSS模式");
            _locationMode = LocationMode::GNSS_ONLY;
            _gnssEnabled = true;
            _lbsEnabled = false;
        #endif
    #else
        debugPrint("未启用定位功能");
        _locationMode = LocationMode::NONE;
    #endif
#else
    debugPrint("未配置定位模块");
    _locationMode = LocationMode::NONE;
#endif
}

// 处理GNSS数据更新
void GPSManager::handleGNSSUpdate()
{
    unsigned long now = millis();
    if (now - _lastGNSSUpdate < GNSS_UPDATE_INTERVAL) {
        return;
    }
    
    bool dataUpdated = false;
    
#ifdef USE_AIR780EG_GSM
    if (air780eg_modem.isNetworkReady()) {
        if (air780eg_modem.updateGNSSData()) {
            _gpsData = air780eg_modem.getGPSData();
            dataUpdated = true;
        }
    }
#elif defined(USE_ML307_GSM)
    // ML307暂时不支持GNSS，这里预留接口
    // if (ml307_at.updateGNSSData()) {
    //     _gpsData = ml307_at.getGPSData();
    //     dataUpdated = true;
    // }
#endif
    
    if (dataUpdated) {
        _lastGNSSUpdate = now;
        debugPrint("GNSS数据更新: " + String(_gpsData.latitude, 6) + 
                  ", " + String(_gpsData.longitude, 6));
    }
}

// 处理LBS数据更新
void GPSManager::handleLBSUpdate()
{
    unsigned long now = millis();
    if (now - _lastLBSUpdate < LBS_UPDATE_INTERVAL) {
        return;
    }
    
    bool dataUpdated = false;
    
#ifdef USE_AIR780EG_GSM
    if (air780eg_modem.isNetworkReady()) {
        if (air780eg_modem.updateLBSData()) {
            _lbsData = air780eg_modem.getLBSData();
            dataUpdated = true;
        }
    }
#elif defined(USE_ML307_GSM)
    if (ml307_at.isNetworkReady()) {
        if (ml307_at.updateLBSData()) {
            _lbsData = ml307_at.getLBSData();
            dataUpdated = true;
        }
    }
#endif
    
    if (dataUpdated) {
        _lastLBSUpdate = now;
        debugPrint("LBS数据更新: " + String(_lbsData.latitude, 6) + 
                  ", " + String(_lbsData.longitude, 6));
    }
}

// 处理GPS数据更新
void GPSManager::handleGPSUpdate()
{
    if (!_gpsInterface) {
        return;
    }
    
    unsigned long now = millis();
    if (now - _lastGPSUpdate < GPS_UPDATE_INTERVAL) {
        return;
    }
    
    _gpsInterface->loop();
    
    if (_gpsInterface->isReady()) {
        _lastGPSUpdate = now;
        // GPS数据通过全局变量gps_data获取
        _gpsData = gps_data;
        debugPrint("GPS数据更新: " + String(_gpsData.latitude, 6) + 
                  ", " + String(_gpsData.longitude, 6));
    }
}

// 更新统一的定位数据
void GPSManager::updateLocationData()
{
    // 根据定位模式和数据有效性，选择最佳的定位数据
    switch (_locationMode) {
        case LocationMode::GPS_ONLY:
            // 直接使用GPS数据
            gps_data = _gpsData;
            break;
            
        case LocationMode::GNSS_ONLY:
            // 直接使用GNSS数据
            gps_data = _gpsData;
            break;
            
        case LocationMode::GNSS_WITH_LBS:
            // 优先使用GNSS数据，信号弱时使用LBS数据
            if (is_gps_data_valid(_gpsData) && _gpsData.altitude > 3) {
                gps_data = _gpsData;
            } else if (_lbsData.valid) {
                gps_data = convert_lbs_to_gps(_lbsData);
                debugPrint("使用LBS数据作为定位源");
            }
            break;
            
        case LocationMode::NONE:
        default:
            // 清空数据
            reset_gps_data(gps_data);
            break;
    }
    
    // 更新全局LBS数据
    lbs_data = _lbsData;
}

// 模式切换私有方法
void GPSManager::switchToGPSMode()
{
    debugPrint("切换到GPS模式");
    _gpsEnabled = true;
    _gnssEnabled = false;
    _lbsEnabled = false;
    
#ifdef ENABLE_GPS
    if (!_gpsInterface) {
        _gpsInterface = new ExternalGPSAdapter(gps);
    }
    if (_gpsInterface) {
        _gpsInterface->begin();
    }
#endif
}

void GPSManager::switchToGNSSMode()
{
    debugPrint("切换到GNSS模式");
    _gpsEnabled = false;
    _gnssEnabled = true;
    _lbsEnabled = false;
    
    // 启用GNSS
    setGNSSEnabled(true);
}

void GPSManager::switchToGNSSWithLBSMode()
{
    debugPrint("切换到GNSS+LBS模式");
    _gpsEnabled = false;
    _gnssEnabled = true;
    _lbsEnabled = true;
    
    // 启用GNSS和LBS
    setGNSSEnabled(true);
    setLBSEnabled(true);
}

void GPSManager::disableAllModes()
{
    debugPrint("禁用所有定位模式");
    
    // 禁用GPS接口
    if (_gpsInterface) {
        delete _gpsInterface;
        _gpsInterface = nullptr;
    }
    
    // 禁用GNSS和LBS
    if (_gnssEnabled) {
        setGNSSEnabled(false);
    }
    if (_lbsEnabled) {
        setLBSEnabled(false);
    }
    
    _gpsEnabled = false;
    _gnssEnabled = false;
    _lbsEnabled = false;
}

// 全局GPS管理器实例
GPSManager &gpsManager = GPSManager::getInstance();
