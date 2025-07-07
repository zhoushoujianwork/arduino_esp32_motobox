#!/usr/bin/env python3
"""
Air780EG库编译测试脚本

这个脚本用于测试Air780EG库是否能正确编译
"""

import os
import subprocess
import sys
import tempfile
import shutil

def create_test_project():
    """创建临时测试项目"""
    test_dir = tempfile.mkdtemp(prefix="air780eg_test_")
    
    # 创建基本的PlatformIO项目结构
    os.makedirs(os.path.join(test_dir, "src"))
    os.makedirs(os.path.join(test_dir, "lib"))
    
    # 复制Air780EG库
    lib_src = "lib/Air780EG"
    lib_dst = os.path.join(test_dir, "lib", "Air780EG")
    shutil.copytree(lib_src, lib_dst)
    
    # 创建platformio.ini
    platformio_ini = """[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
lib_deps = 
build_flags = 
    -DCORE_DEBUG_LEVEL=3
"""
    
    with open(os.path.join(test_dir, "platformio.ini"), "w") as f:
        f.write(platformio_ini)
    
    # 创建测试main.cpp
    test_main = """#include <Arduino.h>
#include <Air780EG.h>

Air780EG air780;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("Air780EG Library Compile Test");
    
    // 测试库初始化
    Air780EG::setLogLevel(AIR780EG_LOG_INFO);
    Air780EG::printVersion();
    
    // 注意：这里不实际初始化硬件，只测试编译
    Serial.println("Compile test completed successfully!");
}

void loop() {
    // 测试主要API调用（编译时检查）
    static bool test_done = false;
    if (!test_done) {
        test_done = true;
        
        // 测试各种API调用
        Serial.println("Testing API calls...");
        
        // 这些调用在没有硬件时会失败，但可以测试编译
        bool network_ready = air780.getNetwork().isNetworkRegistered();
        bool gnss_fixed = air780.getGNSS().isFixed();
        
        Serial.printf("Network ready: %s\\n", network_ready ? "Yes" : "No");
        Serial.printf("GNSS fixed: %s\\n", gnss_fixed ? "Yes" : "No");
        
        Serial.println("API test completed!");
    }
    
    delay(1000);
}
"""
    
    with open(os.path.join(test_dir, "src", "main.cpp"), "w") as f:
        f.write(test_main)
    
    return test_dir

def run_compile_test(test_dir):
    """运行编译测试"""
    print(f"Testing compilation in: {test_dir}")
    
    try:
        # 切换到测试目录
        original_dir = os.getcwd()
        os.chdir(test_dir)
        
        # 运行编译
        print("Running PlatformIO compile...")
        result = subprocess.run(
            ["pio", "run"],
            capture_output=True,
            text=True,
            timeout=300  # 5分钟超时
        )
        
        if result.returncode == 0:
            print("✅ Compilation successful!")
            return True
        else:
            print("❌ Compilation failed!")
            print("STDOUT:", result.stdout)
            print("STDERR:", result.stderr)
            return False
            
    except subprocess.TimeoutExpired:
        print("❌ Compilation timeout!")
        return False
    except Exception as e:
        print(f"❌ Error during compilation: {e}")
        return False
    finally:
        os.chdir(original_dir)

def main():
    print("=== Air780EG Library Compile Test ===")
    
    # 检查是否在正确的目录
    if not os.path.exists("lib/Air780EG"):
        print("❌ Air780EG library not found!")
        print("Please run this script from the project root directory.")
        sys.exit(1)
    
    # 检查PlatformIO是否安装
    try:
        subprocess.run(["pio", "--version"], capture_output=True, check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("❌ PlatformIO not found!")
        print("Please install PlatformIO first: pip install platformio")
        sys.exit(1)
    
    # 创建测试项目
    print("Creating test project...")
    test_dir = create_test_project()
    
    try:
        # 运行编译测试
        success = run_compile_test(test_dir)
        
        if success:
            print("\\n🎉 Air780EG library compile test PASSED!")
            print("The library is ready to use in your project.")
        else:
            print("\\n💥 Air780EG library compile test FAILED!")
            print("Please check the error messages above.")
            sys.exit(1)
            
    finally:
        # 清理临时目录
        print(f"Cleaning up test directory: {test_dir}")
        shutil.rmtree(test_dir, ignore_errors=True)

if __name__ == "__main__":
    main()
