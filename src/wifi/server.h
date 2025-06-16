#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WebServer.h>
#include <DNSServer.h>
#include <WiFi.h>
#include "device.h"
#include "config.h"
#include "esp_wifi.h"
#include "utils/PreferencesUtils.h"
#include "power/PowerManager.h"
#include "wifi/wifi_config_page.h"

class WiFiConfigManager
{
private:
    WebServer server;
    DNSServer dnsServer;
    bool isConfigMode = false;

    // 配置常量
    static constexpr const char *AP_PASSWORD = "11111111";
    String apSSID;

    // 内部方法
    void setupAP();
    void setupDNS();
    void setupWebServer();
    void handleConfigSubmit();
    String getConfigPage();
    String getSuccessPage();
    void saveWiFiCredentials(const String &ssid, const String &password);
    void handleClient();
    bool isIp(String str);
    void startAP();
    void stopAP();
public:
    WiFiConfigManager();
    
    // 核心功能
    void enterConfigMode();  // 进入配网模式
    void exitConfigMode();   // 退出配网模式
    void reset();           // 重置所有WiFi配置
    void setConfigMode(bool mode);

    // 状态查询
    bool getConfigMode() const { return isConfigMode; }
    bool isAPModeActive() const;
    bool hasAPClient() const;

    
    // 安全关闭
    void safeDisableWiFi();

    // 添加loop函数声明
    void loop();
};

#ifdef ENABLE_WIFI
extern WiFiConfigManager wifiManager;
#endif

#endif // WIFI_MANAGER_H
