import subprocess
import os

FILENAME_BUILDNO = 'versioning'
FILENAME_VERSION_H = 'src/version.h'  # Changed to src/version.h to match project structure

def get_latest_tag():
    try:
        tag = subprocess.check_output(['git', 'describe', '--tags', '--abbrev=0']).decode().strip()
    except Exception as e:
        print("获取 git tag 失败: %s" % e)
        tag = "v0.0.0"
    return tag

def get_and_update_buildno():
    build_no = 1
    if os.path.exists(FILENAME_BUILDNO):
        try:
            with open(FILENAME_BUILDNO) as f:
                build_no = int(f.readline().strip()) + 1
        except Exception as e:
            print('读取 build 号失败，重置为 1:', e)
            build_no = 1
    with open(FILENAME_BUILDNO, 'w+') as f:
        f.write(str(build_no))
        print('Build number: {}'.format(build_no))
    return build_no

def write_version_h(tag, build_no):
    version_full = f"{tag}+{build_no}"
    hf = f"""
#ifndef BUILD_NUMBER
  #define BUILD_NUMBER "{build_no}"
#endif
#ifndef VERSION
  #define VERSION "{version_full}"
#endif
#ifndef VERSION_SHORT
  #define VERSION_SHORT "{version_full}"
#endif
"""
    os.makedirs(os.path.dirname(FILENAME_VERSION_H), exist_ok=True)
    with open(FILENAME_VERSION_H, 'w+') as f:
        f.write(hf)
    print(f"写入 {FILENAME_VERSION_H} 完成: {version_full}")

# Standalone execution function
def run_standalone():
    tag = get_latest_tag()
    build_no = get_and_update_buildno()
    write_version_h(tag, build_no)

if __name__ == "__main__":
    run_standalone()

# PlatformIO integration function
def env_inject_version(env):
    version = get_latest_tag()
    hardware = env["PIOENV"]  # 获取当前环境名
    print("Injecting FIRMWARE_VERSION:", version)
    print("Injecting HARDWARE_VERSION:", hardware)

    # Add version definitions directly to build flags
    env.Append(CPPDEFINES=[
        ("FIRMWARE_VERSION", '\\"%s\\"' % version),
        ("HARDWARE_VERSION", '\\"%s\\"' % hardware)
    ])
    
    # Also generate version.h file
    build_no = get_and_update_buildno()
    write_version_h(version, build_no)
    return 0

# This is the function that PlatformIO calls
def before_build(env, platform):
    return env_inject_version(env)
