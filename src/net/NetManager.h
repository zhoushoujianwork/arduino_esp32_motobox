// 网络管理器，统一管理 WiFi 和 4G 网络，提供 Client* 接口
#pragma once
#include <Client.h>

class NetManager {
public:
    enum NetType { NET_NONE, NET_WIFI, NET_GSM };
    using NetworkChangedCallback = void(*)(NetType newType);

    void begin();
    void loop();
    Client* getActiveClient(); // 获取当前可用的 Client
    bool isConnected();        // 当前是否有可用网络
    void selectNetworkType(NetType type); // 选择要使用的网络类型
    void setNetworkChangedCallback(NetworkChangedCallback cb);
    NetType getCurrentType();
private:
    NetType selectedType = NET_WIFI;  // 当前选择的网络类型
    NetworkChangedCallback callback = nullptr;
}; 

extern NetManager netManager;