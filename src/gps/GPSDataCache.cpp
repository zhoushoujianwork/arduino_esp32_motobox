/*
 * GPS数据缓存管理器实现
 * 统一管理GPS数据的定时获取和缓存，避免频繁AT调用
 */

#include "GPSDataCache.h"
#include "../utils/DebugUtils.h"

#ifdef USE_AIR780EG_GSM
extern Air780EGModem air780eg_modem;
#endif

// 全局实例
GPSDataCache& gpsDataCache = GPSDataCache::getInstance();

GPSDataCache& GPSDataCache::getInstance() {
    static GPSDataCache instance;
    return instance;
}

void GPSDataCache::init() {
    if (_initialized) {
        return;
    }
    
    debugPrint("初始化GPS数据缓存管理器");
    
    // 重置缓存数据
    resetCachedData();
    
#ifdef USE_AIR780EG_GSM
    _modem = &air780eg_modem;
    debugPrint("使用Air780EG模块");
#endif
    
    _initialized = true;
    debugPrint("GPS数据缓存管理器初始化完成");
}

void GPSDataCache::loop() {
    if (!_initialized) {
        return;
    }
    
    unsigned long now = millis();
    
    // 检查是否需要更新数据
    if (now - _lastUpdateTime < _updateInterval) {
        return;
    }
    
    // 更新GPS数据
    updateGPSDataFromModem();
    _lastUpdateTime = now;
}

void GPSDataCache::updateGPSDataFromModem() {
#ifdef USE_AIR780EG_GSM
    if (!_modem || !_modem->isGNSSEnabled()) {
        resetCachedData();
        return;
    }
    
    // 直接调用AT+CGNSINF获取数据
    String response = _modem->sendATWithResponse("AT+CGNSINF", 3000);
    if (response.length() == 0) {
        debugPrint("AT+CGNSINF 无响应");
        _dataValid = false;
        return;
    }
    
    // 解析响应数据
    if (parseGNSSResponse(response)) {
        _dataValid = true;
        if (_debug && _gnssFixed) {
            debugPrint(String("GPS数据更新: ") + 
                      String(_cachedGPSData.latitude, 6) + ", " + 
                      String(_cachedGPSData.longitude, 6) + 
                      ", 卫星: " + String(_satelliteCount));
        }
    } else {
        _dataValid = false;
        debugPrint("GPS数据解析失败");
    }
#else
    resetCachedData();
#endif
}

bool GPSDataCache::parseGNSSResponse(const String& response) {
    // 查找数据起始位置
    int start = response.indexOf("+CGNSINF: ") + 10;
    if (start < 10) {
        debugPrint("GNSS响应格式错误");
        return false;
    }
    
    String data = response.substring(start);
    data.trim();
    
    // 分割数据字段
    String fields[22];
    int fieldCount = 0;
    int lastPos = 0;
    
    for (int i = 0; i < data.length() && fieldCount < 21; i++) {
        if (data.charAt(i) == ',') {
            fields[fieldCount] = data.substring(lastPos, i);
            lastPos = i + 1;
            fieldCount++;
        }
    }
    // 最后一个字段
    if (fieldCount < 22) {
        fields[fieldCount] = data.substring(lastPos);
        fieldCount++;
    }
    
    if (fieldCount < 15) {
        debugPrint("GNSS字段数量不足: " + String(fieldCount));
        return false;
    }
    
    // 解析关键字段
    // 字段1: GNSS运行状态 (0=关闭, 1=开启)
    // 字段2: 定位状态 (0=未定位, 1=已定位)
    _gnssFixed = (fields[1] == "1");
    
    if (!_gnssFixed) {
        resetCachedData();
        return true; // 返回true表示解析成功，但未定位
    }
    
    // 解析定位数据
    _cachedGPSData.latitude = fields[3].toDouble();
    _cachedGPSData.longitude = fields[4].toDouble();
    _cachedGPSData.altitude = fields[5].toFloat();
    _cachedGPSData.speed = fields[6].toFloat();
    _cachedGPSData.heading = fields[7].toFloat();
    
    // 解析时间 (格式: yyyyMMddhhmmss.sss)
    String timestamp = fields[2];
    if (timestamp.length() >= 14) {
        _cachedGPSData.year = timestamp.substring(0, 4).toInt();
        _cachedGPSData.month = timestamp.substring(4, 6).toInt();
        _cachedGPSData.day = timestamp.substring(6, 8).toInt();
        _cachedGPSData.hour = timestamp.substring(8, 10).toInt();
        _cachedGPSData.minute = timestamp.substring(10, 12).toInt();
        _cachedGPSData.second = timestamp.substring(12, 14).toInt();
    }
    
    // 解析卫星信息
    _satelliteCount = fields[14].toInt(); // 可见卫星数
    _cachedGPSData.satellites = _satelliteCount;
    
    // 解析HDOP
    _hdop = fields[10].toFloat();
    _cachedGPSData.hdop = _hdop;
    
    return true;
}

void GPSDataCache::resetCachedData() {
    reset_gps_data(_cachedGPSData);
    _dataValid = false;
    _gnssFixed = false;
    _satelliteCount = 0;
    _hdop = 0.0f;
}

void GPSDataCache::debugPrint(const String& message) {
    if (_debug) {
        Serial.println("[GPS-Cache] " + message);
    }
}
