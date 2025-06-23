/*
 * GPS管理器实现 - 支持GPS和LBS双重定位
 */

#include "GPSManager.h"

#ifdef ENABLE_GSM
#include "GSMGNSSAdapter.h"
#include "../net/Ml307AtModem.h"
extern Ml307AtModem ml307;
#elif defined(ENABLE_GPS)
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
    if (_initialized)
        return;

#ifdef ENABLE_GSM
    _gpsInterface = new GSMGNSSAdapter(ml307);
    _lbsEnabled = true; // ML307A支持LBS
#elif defined(ENABLE_GPS)
    _gpsInterface = new ExternalGPSAdapter(gps);
    _lbsEnabled = false; // 外部GPS不支持LBS
#else
    _gpsInterface = nullptr;
    _lbsEnabled = false;
#endif

    if (_gpsInterface)
    {
        _gpsInterface->begin();
        _initialized = true;
    }

    // 初始化LBS数据
    _lbsData.valid = false;
    _lastLBSUpdate = 0;
}

void GPSManager::loop()
{
    if (_gpsInterface)
    {
        _gpsInterface->loop();
        
        // 获取GPS数据
        gps_data_t gpsData = _gpsInterface->getData();
        
        // GPS信号弱时处理LBS
        if (gpsData.satellites <= 3) {
            handleLBSUpdate();
        }
    } else if (_lbsEnabled) {
        // 没有GPS时只处理LBS
        handleLBSUpdate();
    }
}

bool GPSManager::isReady()
{
    // 检查GPS信号质量
    bool gpsReady = false;
    if (_gpsInterface && _gpsInterface->isReady()) {
        gps_data_t data = _gpsInterface->getData();
        // GPS信号强度足够（卫星数大于3）时使用GPS
        gpsReady = data.satellites > 3;
    }
    
    // 当GPS信号弱或不可用时，检查LBS是否可用
    bool lbsReady = false;
    if (!gpsReady && _lbsEnabled) {
        lbsReady = isLBSReady();
    }
    
    return gpsReady || lbsReady;
}

gps_data_t GPSManager::getData()
{
    if (_gpsInterface) {
        gps_data_t gpsData = _gpsInterface->getData();
        
        // 检查GPS信号质量
        if (gpsData.satellites > 3) {
            gpsData.gpsType = "GPS";
            return gpsData;
        }
        
        // GPS信号弱，尝试使用LBS
        if (_lbsEnabled && isLBSReady()) {
            lbs_data_t lbsData = getLBSData();
            if (lbsData.valid) {
                gps_data_t convertedData = convertLBSToGPS(lbsData);
                convertedData.gpsType = "LBS";
                return convertedData;
            }
        }
        
        // 如果都不可用，返回无效数据
        gpsData.valid = false;
        return gpsData;
    }
    
    // 如果没有GPS接口，直接尝试LBS
    if (_lbsEnabled && isLBSReady()) {
        lbs_data_t lbsData = getLBSData();
        if (lbsData.valid) {
            gps_data_t convertedData = convertLBSToGPS(lbsData);
            convertedData.gpsType = "LBS";
            return convertedData;
        }
    }
    
    // 都不可用时返回无效数据
    gps_data_t emptyData;
    memset(&emptyData, 0, sizeof(gps_data_t));
    emptyData.valid = false;
    return emptyData;
}

bool GPSManager::isLBSReady() const
{
    if (!_lbsEnabled)
    {
        return false;
    }
#ifdef ENABLE_GSM
    return ml307.getLBSData().valid;
#else
    return false;
#endif
}

lbs_data_t GPSManager::getLBSData()
{
    if (!_lbsEnabled)
    {
        lbs_data_t empty;
        empty.valid = false;
        return empty;
    }

#ifdef ENABLE_GSM
    // 获取简化的LBS数据并转换为lbs_data_t格式
    lbs_data_t simpleLBS = ml307.getLBSData();
    lbs_data_t lbsData;
    lbsData.valid = simpleLBS.valid;
    lbsData.latitude = simpleLBS.latitude;
    lbsData.longitude = simpleLBS.longitude;
    lbsData.timestamp = simpleLBS.timestamp;
    return lbsData;
#else
    lbs_data_t empty;
    empty.valid = false;
    return empty;
#endif
}

bool GPSManager::hasLBSBackup()
{
    return _lbsEnabled && isLBSReady();
}

void GPSManager::updateLBSData()
{
    if (!_lbsEnabled)
    {
        return;
    }

#ifdef ENABLE_GSM
    // 获取最新的LBS数据
    lbs_data_t simpleLBS = ml307.getLBSData();
    if (simpleLBS.valid)
    {
        _lbsData.valid = simpleLBS.valid;
        _lbsData.latitude = simpleLBS.latitude;
        _lbsData.longitude = simpleLBS.longitude;
        _lbsData.timestamp = simpleLBS.timestamp;
        _lastLBSUpdate = millis();
    }
#endif
}

gps_data_t GPSManager::convertLBSToGPS(const lbs_data_t &lbsData)
{
    gps_data_t gpsData;
    memset(&gpsData, 0, sizeof(gps_data_t));

    if (!lbsData.valid)
    {
        gpsData.valid = false;
        gpsData.gpsType = "LBS";
        return gpsData;
    }

    // 转换LBS数据到GPS格式
    gpsData.valid = true;
    gpsData.latitude = lbsData.latitude;
    gpsData.longitude = lbsData.longitude;
    gpsData.gpsType = "LBS";
    // LBS无以下信息，全部置为无效/0
    gpsData.altitude = 0;
    gpsData.speed = 0;
    gpsData.heading = 0;
    gpsData.satellites = 0;
    gpsData.satellites_used = 0;
    gpsData.year = 0;
    gpsData.month = 0;
    gpsData.day = 0;
    gpsData.hour = 0;
    gpsData.minute = 0;
    gpsData.second = 0;
    gpsData.centisecond = 0;
    gpsData.hdop = 0;
    gpsData.pdop = 0;
    gpsData.vdop = 0;
    gpsData.cn0_max = 0;
    gpsData.hpa = 0;
    gpsData.vpa = 0;
    gpsData.gpsHz = 0;
    return gpsData;
}

void GPSManager::setDebug(bool debug)
{
    if (_gpsInterface)
    {
        _gpsInterface->setDebug(debug);
    }
}

String GPSManager::getType() const
{
    if (_gpsInterface && _gpsInterface->isReady())
    {
        return _gpsInterface->getType();
    }

    if (isLBSReady())
    {
        return "LBS";
    }

    return "None";
}

void GPSManager::setGNSSEnabled(bool enable)
{
#ifdef ENABLE_GSM
    if (_gpsInterface)
    {
        ml307.enableGNSS(enable);
    }
#endif
}

void GPSManager::setLBSEnabled(bool enable)
{
#ifdef ENABLE_GSM
    if (_gpsInterface)
    {
        _lbsEnabled = enable;
        while (!ml307.enableLBS(enable))
        {
            Serial.println("重试启用LBS...");
            delay(1000);
        }
    }
#endif
}

bool GPSManager::isGNSSEnabled() const
{
#ifdef ENABLE_GSM
    return ml307.isGNSSEnabled();
#else
    return false;
#endif
}

bool GPSManager::isLBSEnabled() const
{
#ifdef ENABLE_GSM
    return _lbsEnabled && ml307.isLBSEnabled();
#else
    return false;
#endif
}

// 全局GPS管理器实例
GPSManager &gpsManager = GPSManager::getInstance();

void GPSManager::handleLBSUpdate() {
    if (!_lbsEnabled) {
        return;
    }
#ifdef ENABLE_GSM
    if (millis() - _lastLBSRequest >= LBS_UPDATE_INTERVAL) {
        ml307.updateLBSData();
        _lbsData = ml307.getLBSData();
        _lastLBSRequest = millis();
    }
#endif
}