/*
 * Air780EG GNSS适配器实现
 * 让GPSManager能够统一管理Air780EG的GNSS功能
 */

#include "Air780EGGNSSAdapter.h"

#ifdef USE_AIR780EG_GSM

extern Air780EGModem air780eg_modem;

Air780EGGNSSAdapter::Air780EGGNSSAdapter(Air780EGModem& modem)
    : _modem(modem), _debug(false), _initialized(false), _gnssEnabled(false), _lastUpdate(0)
{
    reset_gps_data(_gpsData);
}

void Air780EGGNSSAdapter::begin()
{
    GNSS_DEBUG_PRINTLN("[Air780EG-GNSS] 开始初始化GNSS适配器");
    
    if (!_modem.isNetworkReady()) {
        GNSS_DEBUG_PRINTLN("[Air780EG-GNSS] 等待网络就绪...");
        return;
    }
    
    // 启用GNSS功能
    if (_modem.enableGNSS(true)) {
        _gnssEnabled = true;
        _initialized = true;
        GNSS_DEBUG_PRINTLN("[Air780EG-GNSS] GNSS适配器初始化成功");
    } else {
        GNSS_DEBUG_PRINTLN("[Air780EG-GNSS] GNSS启用失败");
    }
}

void Air780EGGNSSAdapter::loop()
{
    if (!_initialized || !_gnssEnabled) {
        return;
    }
    
    unsigned long now = millis();
    if (now - _lastUpdate < UPDATE_INTERVAL) {
        return;
    }
    
    // 更新GNSS数据
    if (_modem.updateGNSSData()) {
        updateGPSData();
        _lastUpdate = now;
        
        if (_debug && is_gps_data_valid(_gpsData)) {
            GNSS_DEBUG_PRINTF("[Air780EG-GNSS] 位置更新: %.6f, %.6f, 高度: %.1fm, 卫星: %d\n",
                             _gpsData.latitude, _gpsData.longitude, _gpsData.altitude, _gpsData.satellites);
        }
    }
}

bool Air780EGGNSSAdapter::isReady()
{
    if (!_initialized || !_gnssEnabled) {
        return false;
    }
    
    return _modem.isGNSSFixed() && is_gps_data_valid(_gpsData);
}

void Air780EGGNSSAdapter::setDebug(bool debug)
{
    _debug = debug;
    _modem.setDebug(debug);
    debugPrint(String("调试模式 ") + (debug ? "启用" : "禁用"));
}

String Air780EGGNSSAdapter::getType() const
{
    return "Air780EG-GNSS";
}

bool Air780EGGNSSAdapter::enableGNSS(bool enable)
{
    if (_gnssEnabled == enable) {
        return true;
    }
    
    debugPrint(String("GNSS ") + (enable ? "启用" : "禁用"));
    
    if (_modem.enableGNSS(enable)) {
        _gnssEnabled = enable;
        if (!enable) {
            reset_gps_data(_gpsData);
        }
        return true;
    }
    
    return false;
}

bool Air780EGGNSSAdapter::isGNSSEnabled() const
{
    return _gnssEnabled && _modem.isGNSSEnabled();
}

bool Air780EGGNSSAdapter::isGNSSFixed() const
{
    return _modem.isGNSSFixed();
}

int Air780EGGNSSAdapter::getSatelliteCount() const
{
    return _gpsData.satellites;
}

float Air780EGGNSSAdapter::getHDOP() const
{
    return _gpsData.hdop;
}

bool Air780EGGNSSAdapter::setUpdateRate(int hz)
{
    debugPrint("设置GNSS更新频率: " + String(hz) + "Hz");
    return _modem.setGNSSUpdateRate(hz);
}

String Air780EGGNSSAdapter::getGNSSInfo() const
{
    return _modem.getGNSSInfo();
}

String Air780EGGNSSAdapter::getRawData() const
{
    return _modem.getGNSSRawData();
}

gps_data_t Air780EGGNSSAdapter::getGPSData() const
{
    return _gpsData;
}

bool Air780EGGNSSAdapter::isGPSDataValid() const
{
    return is_gps_data_valid(_gpsData);
}

// 私有方法实现

void Air780EGGNSSAdapter::debugPrint(const String& message)
{
    if (_debug) {
        GNSS_DEBUG_PRINTLN("[Air780EG-GNSS] " + message);
    }
}

void Air780EGGNSSAdapter::updateGPSData()
{
    // 从Air780EG获取GNSS数据并转换为GPS格式
    GNSSData gnssData = _modem.getGNSSData();
    _gpsData = convertGNSSToGPS(gnssData);
    
    // 更新全局GPS数据
    if (is_gps_data_valid(_gpsData)) {
        gps_data = _gpsData;
    }
}

gps_data_t Air780EGGNSSAdapter::convertGNSSToGPS(const GNSSData& gnssData)
{
    gps_data_t gpsData;
    reset_gps_data(gpsData);
    
    if (gnssData.valid) {
        gpsData.latitude = gnssData.latitude;
        gpsData.longitude = gnssData.longitude;
        gpsData.altitude = gnssData.altitude;
        gpsData.speed = gnssData.speed;
        gpsData.heading = gnssData.course;  // course -> heading
        gpsData.satellites = gnssData.satellites;
        gpsData.hdop = gnssData.hdop;
        
        // 设置时间戳
        if (!gnssData.timestamp.isEmpty()) {
            // 解析时间戳格式，这里简化处理
            gpsData.hour = 0;
            gpsData.minute = 0;
            gpsData.second = 0;
            gpsData.day = 1;
            gpsData.month = 1;
            gpsData.year = 2024;
        }
    }
    
    return gpsData;
}

#endif // USE_AIR780EG_GSM
