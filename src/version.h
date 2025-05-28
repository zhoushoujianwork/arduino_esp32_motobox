#ifndef VERSION_H
#define VERSION_H

#include "config.h"

// 版本信息结构体
struct VersionInfo {
    const char* hardware_version;      // 硬件版本
    const char* firmware_version;    // 固件版本
    const char* build_time;    // 编译时间
};

// 获取版本信息
const VersionInfo& getVersionInfo();

#endif // VERSION_H 