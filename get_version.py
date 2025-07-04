# -*- coding: utf-8 -*-
import subprocess
import os

# 导入PlatformIO环境
try:
    Import("env")
    PLATFORMIO_MODE = True
except:
    PLATFORMIO_MODE = False

FILENAME_BUILDNO = 'versioning'
FILENAME_VERSION_H = 'src/version.h'

def get_latest_tag():
    try:
        # 在PlatformIO环境中使用当前工作目录
        cwd = os.getcwd() if PLATFORMIO_MODE else os.path.dirname(os.path.abspath(__file__))
        tag = subprocess.check_output(['git', 'describe', '--tags', '--abbrev=0'], cwd=cwd).decode().strip()
        print("Git tag found: %s" % tag)
    except Exception as e:
        print("Failed to get git tag: %s" % e)
        tag = "v0.0.0"
    return tag

def get_and_update_buildno():
    build_no = 1
    # 在PlatformIO环境中使用当前工作目录
    if PLATFORMIO_MODE:
        build_file = os.path.join(os.getcwd(), FILENAME_BUILDNO)
    else:
        build_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), FILENAME_BUILDNO)
    
    if os.path.exists(build_file):
        try:
            with open(build_file, 'r') as f:
                build_no = int(f.readline().strip()) + 1
        except Exception as e:
            print('Failed to read build number, reset to 1:', e)
            build_no = 1
    
    with open(build_file, 'w') as f:
        f.write(str(build_no))
        print('Build number updated to: {}'.format(build_no))
    return build_no

def write_version_h(tag, build_no, hardware="lite"):
    version_full = "{0}+{1}".format(tag, build_no)
    
    version_h_content = """#ifndef VERSION_H
#define VERSION_H

// 自动生成的版本号宏
#ifndef BUILD_NUMBER
  #define BUILD_NUMBER "{0}"
#endif
#ifndef FIRMWARE_VERSION
  #define FIRMWARE_VERSION "{1}"
#endif
#ifndef HARDWARE_VERSION
  #define HARDWARE_VERSION "{2}"
#endif

// 版本信息结构体
typedef struct {{
    const char* hardware_version;
    const char* firmware_version;
    const char* build_time;
}} VersionInfo;

// 获取版本信息函数声明
const VersionInfo& getVersionInfo();

#endif // VERSION_H
""".format(build_no, version_full, hardware)
    
    # 确保目录存在
    if PLATFORMIO_MODE:
        version_h_path = os.path.join(os.getcwd(), FILENAME_VERSION_H)
    else:
        version_h_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), FILENAME_VERSION_H)
    
    dir_path = os.path.dirname(version_h_path)
    if not os.path.exists(dir_path):
        os.makedirs(dir_path)
        
    with open(version_h_path, 'w') as f:
        f.write(version_h_content)
    print("Version file updated: {0} -> {1}".format(version_h_path, version_full))

def update_version():
    print("=== 版本管理脚本开始执行 ===")
    
    # 获取Git标签
    tag = get_latest_tag()
    
    # 更新构建号
    build_no = get_and_update_buildno()
    
    # 获取硬件版本
    if PLATFORMIO_MODE:
        hardware = env.get('PIOENV', 'lite')
    else:
        hardware = os.environ.get('PIOENV', 'lite')
    
    # 写入版本头文件
    write_version_h(tag, build_no, hardware)
    
    print("=== 版本管理脚本执行完成 ===")
    print("版本: {0}+{1}".format(tag, build_no))
    print("硬件: {0}".format(hardware))
    
    return tag, build_no, hardware

# PlatformIO调用入口
if PLATFORMIO_MODE:
    print("PlatformIO模式：执行版本更新")
    tag, build_no, hardware = update_version()
    
    # 添加编译定义
    env.Append(CPPDEFINES=[
        ("FIRMWARE_VERSION", '\\"%s+%s\\"' % (tag, build_no)),
        ("HARDWARE_VERSION", '\\"%s\\"' % hardware),
        ("BUILD_NUMBER", '\\"%s\\"' % build_no)
    ])
    
    print("已添加编译定义:")
    print("  FIRMWARE_VERSION=%s+%s" % (tag, build_no))
    print("  HARDWARE_VERSION=%s" % hardware)
    print("  BUILD_NUMBER=%s" % build_no)

# 独立执行入口
if __name__ == "__main__":
    print("独立模式：执行版本更新")
    update_version()
