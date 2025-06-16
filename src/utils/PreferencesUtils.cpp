#include "PreferencesUtils.h"

bool PreferencesUtils::_initialized = false;
Preferences PreferencesUtils::_prefs;

bool PreferencesUtils::init() {
    if (_initialized) return true;
    
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS 分区被损坏，需要擦除并重新初始化
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    
    _initialized = true;
    return true;
}

// 只存储单个 WiFi 配置
bool PreferencesUtils::saveWifi(const String& ssid, const String& password) {
    Preferences prefs;
    if (!prefs.begin("wifi", false)) return false;
    bool success = prefs.putString("ssid", ssid) && prefs.putString("pass", password);
    prefs.end();
    return success;
}

bool PreferencesUtils::getWifi(String& ssid, String& password) {
    Preferences prefs;
    if (!prefs.begin("wifi", true)) return false;
    ssid = prefs.getString("ssid", "");
    password = prefs.getString("pass", "");
    prefs.end();
    return ssid.length() > 0;
}

bool PreferencesUtils::clearWifi() {
    Preferences prefs;
    if (!prefs.begin("wifi", false)) return false;
    bool success = prefs.clear();
    prefs.end();
    return success;
}

void PreferencesUtils::saveULong(const char* ns, const char* key, unsigned long value) {
    Preferences prefs;
    prefs.begin(ns, false);
    prefs.putULong(key, value);
    prefs.end();
}

unsigned long PreferencesUtils::loadULong(const char* ns, const char* key, unsigned long defaultValue) {
    Preferences prefs;
    prefs.begin(ns, true);
    unsigned long value = prefs.getULong(key, defaultValue);
    prefs.end();
    return value;
}

void PreferencesUtils::saveString(const char* ns, const char* key, const String& value) {
    Preferences prefs;
    prefs.begin(ns, false);
    prefs.putString(key, value);
    prefs.end();
}

String PreferencesUtils::loadString(const char* ns, const char* key, const String& defaultValue) {
    Preferences prefs;
    prefs.begin(ns, true);
    String value = prefs.getString(key, defaultValue);
    prefs.end();
    return value;
}

// loadSleepTime
unsigned long PreferencesUtils::loadSleepTime() {
    Preferences prefs;
    prefs.begin(NS_POWER, true);
    unsigned long value = prefs.getULong(KEY_SLEEP_TIME, 0);
    prefs.end();
    return value;
}

// saveSleepTime
void PreferencesUtils::saveSleepTime(unsigned long seconds) {
    Preferences prefs;
    prefs.begin(NS_POWER, false);
    prefs.putULong(KEY_SLEEP_TIME, seconds);
    prefs.end();
}