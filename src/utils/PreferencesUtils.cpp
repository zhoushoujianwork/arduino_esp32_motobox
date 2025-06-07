#include "PreferencesUtils.h"

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