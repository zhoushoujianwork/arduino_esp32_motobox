#include "version.h"
#include <Arduino.h>

// 版本信息实例
static VersionInfo versionInfo = {
    .version = VERSION,
    .buildTime = __DATE__ " " __TIME__,
    .buildEnv = PIOENV
};

// 获取版本信息
const VersionInfo& getVersionInfo() {
    return versionInfo;
} 