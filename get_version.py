import subprocess
import os

FILENAME_BUILDNO = 'versioning'
FILENAME_VERSION_H = 'include/version.h'

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

def main():
    tag = get_latest_tag()
    build_no = get_and_update_buildno()
    write_version_h(tag, build_no)

if __name__ == "__main__":
    main()

def main(env, *args, **kwargs):
    version = get_latest_tag()
    hardware = env["PIOENV"]  # 获取当前环境名
    print("Injecting FIRMWARE_VERSION:", version)
    print("Injecting HARDWARE_VERSION:", hardware)

    if version == "unknown" or hardware == "unknown":
        env.Exit(1)
        return -1

    env.Append(CPPDEFINES=[
        ("FIRMWARE_VERSION", '"%s"' % version),
        ("HARDWARE_VERSION", '"%s"' % hardware)
    ])
    return 0

def before_build(env, platform):
    main(env)
