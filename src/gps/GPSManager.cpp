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
    }

    // GPS信号弱时处理LBS- 简化版本
    if (gps_data.altitude <= 3)
    {
        // LBS 都查询
        handleLBSUpdate();
        // 如果LBS有数据，则使用LBS数据
        if (lbs_data.valid)
        {
            // 使用通用的转换函数
            gps_data = convert_lbs_to_gps(lbs_data);
        }else{
            debugPrint("LBS定位失败，继续等待GPS定位");
        }
    }
   
}

bool GPSManager::isReady()
{
    return gps_data.altitude>3 || lbs_data.valid;
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
        // ml307_at.enableGNSS(enable);
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
    // 检查是否需要更新LBS数据
    unsigned long now = millis();
    
    // 统一使用10秒间隔
    if (now - _lastLBSUpdate < 10000) // 10秒间隔
    {
        return;
    }
    
    // 检查网络状态
    if (!ml307_at.isNetworkReady())
    {
        debugPrint("网络未就绪，跳过LBS更新");
        return;
    }
    
    // 检查LBS是否正在加载
    if (ml307_at.isLBSLoading())
    {
        debugPrint("LBS正在加载中，跳过本次更新");
        return;
    }
    
    // 更新LBS数据 - 直接调用Ml307AtModem的方法
    if (ml307_at.updateLBSData())
    {
        _lastLBSUpdate = now;
        
        // 获取解析后的LBS数据
        lbs_data_t lbsData = ml307_at.getLBSData();
        if (ml307_at.isLBSDataValid()) {
            // 更新内部LBS数据
            _lbsData = lbsData;
            debugPrint("LBS数据更新成功: " + String(lbsData.longitude, 6) + 
                      ", " + String(lbsData.latitude, 6));
        } else {
            debugPrint("LBS数据无效");
        }
    } else {
        // 减少失败日志频率，避免刷屏
        static unsigned long lastFailureLog = 0;
        if (now - lastFailureLog > 30000) { // 30秒才打印一次失败日志
            debugPrint("LBS更新失败");
            lastFailureLog = now;
        }
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