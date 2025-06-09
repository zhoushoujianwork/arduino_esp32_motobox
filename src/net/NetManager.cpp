#include "NetManager.h"
#include "wifi/WifiManager.h"
#include "gsm/Ml307AtModem.h"

// 这里假设有全局 WiFi 配置和 GSM 配置参数
extern HardwareSerial Serial2;
// NetManager 成员实例

NetManager netManager;

void NetManager::begin() {
    Serial.println("NetManager::begin");
    ml370.begin();
    wifiManager.begin();
}

void NetManager::loop() {
    // WiFi 状态检查
    bool wifiOk = (WiFi.status() == WL_CONNECTED);
    // GSM 状态检查
    bool gsmOk = ml370.sendAT("AT", "OK", 500);
    NetType newType = NET_NONE;
    if (primaryType == NET_WIFI && wifiOk) {
        newType = NET_WIFI;
    } else if (secondaryType == NET_GSM && gsmOk) {
        newType = NET_GSM;
    } else if (primaryType == NET_GSM && gsmOk) {
        newType = NET_GSM;
    } else if (secondaryType == NET_WIFI && wifiOk) {
        newType = NET_WIFI;
    }
    if (newType != lastType) {
        if (callback) callback(newType);
        lastType = newType;
    }
}

Client* NetManager::getActiveClient() {
    if (lastType == NET_WIFI) {
        return &wifiManager.wifiClient;
    } else if (lastType == NET_GSM) {
        return &ml370;
    }
    return nullptr;
}

bool NetManager::isConnected() {
    return lastType != NET_NONE;
}

void NetManager::setPriority(NetType primary, NetType secondary) {
    primaryType = primary;
    secondaryType = secondary;
}

void NetManager::setNetworkChangedCallback(NetworkChangedCallback cb) {
    callback = cb;
}

NetManager::NetType NetManager::getCurrentType() {
    return lastType;
}
 