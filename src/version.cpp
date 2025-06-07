#include "version.h"
#include <Arduino.h>

// 版本信息实例 - 使用静态定义避免每次调用都创建新实例
static const VersionInfo versionInfo = {
    .hardware_version = HARDWARE_VERSION,
    .firmware_version = FIRMWARE_VERSION,
    .build_time = __DATE__ " " __TIME__,
};

// 获取版本信息
const VersionInfo& getVersionInfo() {
    return versionInfo;
}
