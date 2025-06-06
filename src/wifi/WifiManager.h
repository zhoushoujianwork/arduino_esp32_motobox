#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "device.h"
#include "config.h"
#include "esp_wifi.h"

class WiFiConfigManager
{
private:
    WebServer server;
    DNSServer dnsServer;
    Preferences preferences;

    // 配置常量
    static constexpr const char *PREF_NAMESPACE = "wifi-config";
    static constexpr const char *AP_PASSWORD = "11111111";
    static constexpr uint16_t CONNECT_TIMEOUT_MS = 10000; // 10秒连接超时

    String apSSID;
    bool isConfigMode = false;

    // 核心功能方法
    bool tryConnectWithSavedCredentials();
    bool checkInternetConnection();
    void handleClient();

    // 配置模式相关方法
    void setupAP();
    void setupDNS();
    void setupWebServer();
    void handleConfigSubmit();
    String getConfigPage();
    void saveWiFiCredentials(const String &ssid, const String &password);

    // 新增辅助方法
    bool isIp(String str);

    // 新增：单热点连接测试
    bool tryConnectSingle(const String& ssid, const String& password);

   

public:
    WiFiClientSecure wifiClientSecure;
    WiFiClient wifiClient;
    void begin();
    void loop();
    void enterConfigMode();
    void exitConfigMode();
    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
    void reset();
    bool getConfigMode() const { return isConfigMode; }
    String getSuccessPage();

     // 新增：AP模式和客户端判断
    bool isAPModeActive() const;
    bool hasAPClient() const;

    void safeDisableWiFi();      // 安全关闭WiFi

};

// 全局实例和接口函数
extern WiFiConfigManager wifiManager;

#endif // WIFI_MANAGER_H
