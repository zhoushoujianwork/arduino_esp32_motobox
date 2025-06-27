/*
 * GPS管理器实现 - 支持GPS和LBS双重定位
 */

#include "GPSManager.h"

#ifdef ENABLE_GSM
#include "GSMGNSSAdapter.h"
#include "../net/Ml307AtModem.h"
extern Ml307AtModem ml307_at;
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
    _debug = true;
    Serial.println("[GPSManager] 开始初始化");
#ifdef ENABLE_GPS
    // 外部GPS模式
    Serial.println("[GPSManager] 外部GPS模式");
    _gpsInterface = new ExternalGPSAdapter(gps);
#else
    // GSM模式：使用ML307模块 GPS和GNSS 2 选一的模块
    _gpsInterface = nullptr; // 暂时不创建GPS接口
#endif

    if (_gpsInterface)
    {
        Serial.println("[GPSManager] 开始初始化GPS");
        _gpsInterface->begin();
        _initialized = true;
    }

    // 初始化LBS数据
    _lbsData.valid = false;
    _lastLBSUpdate = 0;
}

/*
GPS和GNSS 是 2 选一的情况
LBS 是都会请求执行参考，让信号很差的时候用
*/
void GPSManager::loop()
{
    if (_gpsInterface)
    {
        _gpsInterface->loop();

        // 获取GPS数据
        gps_data_t gpsData = _gpsInterface->getData();

        // GPS信号弱时处理GNSS- 简化版本
        if (gpsData.satellites <= 3)
        {
            debugPrint("GPS信号弱，开始GNSS定位");
        }
    }
    // LBS 都查询
    handleLBSUpdate();
}

bool GPSManager::isReady()
{
    // 检查GPS信号质量
    bool gpsReady = false;
    if (_gpsInterface && _gpsInterface->isReady())
    {
        gps_data_t data = _gpsInterface->getData();
        // GPS信号强度足够（卫星数大于3）时使用GPS
        gpsReady = data.satellites > 3;
    }

    // 当GPS信号弱或不可用时，检查LBS是否可用
    bool lbsReady = false;

    return gpsReady || lbsReady;
}

gps_data_t GPSManager::getData()
{
    // 都不可用时返回无效数据
    gps_data_t emptyData;
    memset(&emptyData, 0, sizeof(gps_data_t));
    return emptyData;
}

void GPSManager::updateLBSData()
{
#ifdef ENABLE_GSM
    debugPrint("TODO:更新LBS数据");
    // 获取最新的LBS数据
#endif
}

gps_data_t GPSManager::convertLBSToGPS(const lbs_data_t &lbsData)
{
    gps_data_t gpsData;
    memset(&gpsData, 0, sizeof(gps_data_t));

    if (!lbsData.valid)
    {
        return gpsData;
    }

    // 转换LBS数据到GPS格式
    gpsData.latitude = lbsData.latitude;
    gpsData.longitude = lbsData.longitude;
    // LBS无以下信息，全部置为无效/0
    gpsData.altitude = 0;
    gpsData.speed = 0;
    gpsData.heading = 0;
    gpsData.satellites = 0;
    gpsData.year = 0;
    gpsData.month = 0;
    gpsData.day = 0;
    gpsData.hour = 0;
    gpsData.minute = 0;
    gpsData.second = 0;
    gpsData.centisecond = 0;
    gpsData.hdop = 0;
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

    return "None";
}

void GPSManager::setGNSSEnabled(bool enable)
{
#ifdef ENABLE_GSM
    if (_gpsInterface)
    {
        ml307_at.enableGNSS(enable);
    }
#endif
}

void GPSManager::setLBSEnabled(bool enable)
{
#ifdef ENABLE_GSM
    while (!ml307_at.enableLBS(enable))
    {
        Serial.println("重试启用LBS...");
        delay(1000);
    }
#endif
}

bool GPSManager::isGNSSEnabled() const
{
    return false;
}

// 全局GPS管理器实例
GPSManager &gpsManager = GPSManager::getInstance();

void GPSManager::handleLBSUpdate()
{
#ifdef ENABLE_GSM
    if (ml307_at.updateLBSData())
    {
        String lbsResponse = ml307_at.getLBSRawData();
        debugPrint("LBS原始数据: " + lbsResponse);
    }
#endif
}

void GPSManager::debugPrint(const String &message)
{
    if (_debug)
    {
        Serial.println("[GPSManager] [debug] " + message);
    }
}