#include "NetManager.h"
#include "wifi/WifiManager.h"
#include "gsm/Ml307AtModem.h"

// GSM引脚配置检查
#if defined(GSM_RX_PIN) && defined(GSM_TX_PIN)
#define GSM_ENABLED 1
#endif

// 这里假设有全局 WiFi 配置和 GSM 配置参数
extern HardwareSerial Serial2;
// NetManager 成员实例

NetManager netManager;

/**
 * @brief 初始化网络管理器
 * 根据编译配置决定是否启用GSM功能
 */
void NetManager::begin() {
    Serial.println("NetManager::begin");
    
    #ifdef GSM_ENABLED
    ml370.begin();
    #endif
    
    wifiManager.begin();
    
    selectedType = NET_WIFI;
}

/**
 * @brief 网络管理器主循环
 * 检查网络状态并在需要时切换网络类型
 */
void NetManager::loop() {
    // 调用WiFi管理器的loop函数进行轮询
    wifiManager.loop();
    
    #ifdef GSM_ENABLED
    bool gsmOk = ml370.isReady();
    // 如果当前是GSM模式但GSM不可用，自动切换到WiFi模式
    if (selectedType == NET_GSM && !gsmOk) {
        selectedType = NET_WIFI;
        Serial.println("GSM lost, switching to WiFi mode");
        if (callback) callback(NET_WIFI);
    }
    #endif
}

/**
 * @brief 获取当前活动的网络客户端
 * @return Client* 返回当前活动的网络客户端指针，如果没有活动连接则返回nullptr
 */
Client* NetManager::getActiveClient() {
    if (selectedType == NET_WIFI && WiFi.status() == WL_CONNECTED) {
        return &wifiManager.wifiClient;
    }
    #ifdef GSM_ENABLED 
    else if (selectedType == NET_GSM && ml370.isReady()) {
        return &ml370;
    }
    #endif
    return nullptr;
}

/**
 * @brief 检查是否有网络连接
 * @return bool 如果有任何类型的网络连接则返回true
 */
bool NetManager::isConnected() {
    if (selectedType == NET_WIFI) {
        return WiFi.status() == WL_CONNECTED;
    }
    #ifdef GSM_ENABLED
    else if (selectedType == NET_GSM) {
        return ml370.isReady();
    }
    #endif
    return false;
}

/**
 * @brief 设置网络变更回调函数
 * @param cb 回调函数
 */
void NetManager::setNetworkChangedCallback(NetworkChangedCallback cb) {
    callback = cb;
}

/**
 * @brief 获取当前网络类型
 * @return NetType 当前网络类型
 */
NetManager::NetType NetManager::getCurrentType() {
    return selectedType;
}

/**
 * @brief 选择要使用的网络类型
 * @param type 网络类型
 */
void NetManager::selectNetworkType(NetType type) {
    selectedType = type;
}
 