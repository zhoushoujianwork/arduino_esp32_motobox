#pragma once
#include <Preferences.h>
#include <Arduino.h>

/**
 * @brief 统一管理Preferences参数存取
 */
class PreferencesUtils {
public:
    // 固定命名空间和Key
    static constexpr const char* NS_WIFI = "wifi-config";
    static constexpr const char* KEY_WIFI_LIST = "wifi_list";
    static constexpr const char* NS_POWER = "power";
    static constexpr const char* KEY_SLEEP_TIME = "sleepTimeSec";

    static void saveULong(const char* ns, const char* key, unsigned long value);
    static unsigned long loadULong(const char* ns, const char* key, unsigned long defaultValue = 0);

    static void saveString(const char* ns, const char* key, const String& value);
    static String loadString(const char* ns, const char* key, const String& defaultValue = "");
}; 