#include "version.h"
#include <Arduino.h>

// 版本信息实例
static VersionInfo versionInfo = {
    .hardware_version = HARDWARE_VERSION,
    .firmware_version = FIRMWARE_VERSION,
    .build_time = __DATE__ " " __TIME__,
};

// 获取版本信息
const VersionInfo& getVersionInfo() {
    return versionInfo;
} 