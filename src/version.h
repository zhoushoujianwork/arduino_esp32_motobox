#ifndef VERSION_H
#define VERSION_H

#include "config.h"

// 版本信息结构体
struct VersionInfo {
    const char* version;      // 版本号
    const char* buildTime;    // 编译时间
    const char* buildEnv;     // 编译环境
};

// 获取版本信息
const VersionInfo& getVersionInfo();

#endif // VERSION_H 