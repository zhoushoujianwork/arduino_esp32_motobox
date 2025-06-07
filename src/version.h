#ifndef VERSION_H
#define VERSION_H

// 自动生成的版本号宏
#ifndef BUILD_NUMBER
  #define BUILD_NUMBER "1"
#endif
#ifndef FIRMWARE_VERSION
  #define FIRMWARE_VERSION "v2.2.2+1"
#endif
#ifndef HARDWARE_VERSION
  #define HARDWARE_VERSION "lite"
#endif

// 版本信息结构体
typedef struct {
    const char* hardware_version;
    const char* firmware_version;
    const char* build_time;
} VersionInfo;

// 获取版本信息函数声明
const VersionInfo& getVersionInfo();

#endif // VERSION_H
