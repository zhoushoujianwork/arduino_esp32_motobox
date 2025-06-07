# -*- coding: utf-8 -*-
import subprocess
import os

FILENAME_BUILDNO = 'versioning'
FILENAME_VERSION_H = 'src/version.h'  # Changed to src/version.h to match project structure

def get_latest_tag():
    try:
        tag = subprocess.check_output(['git', 'describe', '--tags', '--abbrev=0']).decode().strip()
    except Exception as e:
        print("Failed to get git tag: %s" % e)
        tag = "v0.0.0"
    return tag

def get_and_update_buildno():
    build_no = 1
    if os.path.exists(FILENAME_BUILDNO):
        try:
            with open(FILENAME_BUILDNO) as f:
                build_no = int(f.readline().strip()) + 1
        except Exception as e:
            print('Failed to read build number, reset to 1:', e)
            build_no = 1
    with open(FILENAME_BUILDNO, 'w+') as f:
        f.write(str(build_no))
        print('Build number: {}'.format(build_no))
    return build_no

def write_version_h(tag, build_no):
    version_full = "{0}+{1}".format(tag, build_no)
    hf = """
#ifndef BUILD_NUMBER
  #define BUILD_NUMBER "{0}"
#endif
#ifndef VERSION
  #define VERSION "{1}"
#endif
#ifndef VERSION_SHORT
  #define VERSION_SHORT "{1}"
#endif
""".format(build_no, version_full)
    
    # Create directory if it doesn't exist
    dir_path = os.path.dirname(FILENAME_VERSION_H)
    if dir_path and not os.path.exists(dir_path):
        os.makedirs(dir_path)
        
    with open(FILENAME_VERSION_H, 'w+') as f:
        f.write(hf)
    print("Writing {0} completed: {1}".format(FILENAME_VERSION_H, version_full))

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
    hardware = env["PIOENV"]  # Get current environment name
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
