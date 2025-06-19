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

    // 电池校准参数
    static constexpr const char* NS_BATTERY = "battery";
    static constexpr const char* KEY_BAT_MIN = "min_voltage";
    static constexpr const char* KEY_BAT_MAX = "max_voltage";
    
    // 保存电池电压范围
    static void saveBatteryRange(int minVoltage, int maxVoltage) {
        _prefs.begin(NS_BATTERY, false);
        _prefs.putInt(KEY_BAT_MIN, minVoltage);
        _prefs.putInt(KEY_BAT_MAX, maxVoltage);
        _prefs.end();
    }
    
    // 读取电池电压范围
    static void loadBatteryRange(int& minVoltage, int& maxVoltage) {
        _prefs.begin(NS_BATTERY, true);
        minVoltage = _prefs.getInt(KEY_BAT_MIN, 2800);  // 默认值
        maxVoltage = _prefs.getInt(KEY_BAT_MAX, 4200);  // 默认值
        _prefs.end();
    }
    
    // 清除电池校准数据
    static void clearBatteryRange() {
        _prefs.begin(NS_BATTERY, false);
        _prefs.remove(KEY_BAT_MIN);
        _prefs.remove(KEY_BAT_MAX);
        _prefs.end();
    }

    static bool init();
    static bool isInitialized() { return _initialized; }
private:
    static bool _initialized;
    static Preferences _prefs;

}; 