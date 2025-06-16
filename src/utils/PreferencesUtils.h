#pragma once
#include <Preferences.h>
#include <Arduino.h>
#include "nvs_flash.h"

/**
 * @brief 统一管理Preferences参数存取
 */
class PreferencesUtils {
public:
    static void saveULong(const char* ns, const char* key, unsigned long value);
    static unsigned long loadULong(const char* ns, const char* key, unsigned long defaultValue = 0);
    static void saveString(const char* ns, const char* key, const String& value);
    static String loadString(const char* ns, const char* key, const String& defaultValue = "");

    // 单个WiFi配置接口
    static bool saveWifi(const String& ssid, const String& password);
    static bool getWifi(String& ssid, String& password);
    static bool clearWifi();

    // 休眠时间
    static constexpr const char* NS_POWER = "power";
    static constexpr const char* KEY_SLEEP_TIME = "sleepTimeSec";
    static unsigned long loadSleepTime();
    static void saveSleepTime(unsigned long seconds);

    static bool init();
    static bool isInitialized() { return _initialized; }
private:
    static bool _initialized;
    static Preferences _prefs;

}; 