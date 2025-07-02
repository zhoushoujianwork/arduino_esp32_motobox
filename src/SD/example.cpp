/*
 * SD卡管理器使用示例
 * 此文件仅作为参考，不会被编译到项目中
 */

#ifdef ENABLE_SDCARD
#include "SDManager.h"

void sdCardExample() {
    Serial.println("=== SD卡管理器示例 ===");
    
    // 1. 初始化SD卡
    sdManager.setDebug(true);
    if (!sdManager.begin()) {
        Serial.println("SD卡初始化失败，退出示例");
        return;
    }
    
    // 2. 显示SD卡信息
    Serial.println("\n--- SD卡信息 ---");
    sdManager.printCardInfo();
    
    // 3. 文件操作示例
    Serial.println("\n--- 文件操作示例 ---");
    
    // 创建测试目录
    sdManager.createDir("/test");
    
    // 保存测试配置
    String testConfig = "{\"test_mode\":true,\"version\":\"1.0\"}";
    if (sdManager.saveConfig(testConfig, "/test/config.json")) {
        Serial.println("测试配置保存成功");
    }
    
    // 读取测试配置
    String loadedConfig = sdManager.loadConfig("/test/config.json");
    if (!loadedConfig.isEmpty()) {
        Serial.println("读取到配置: " + loadedConfig);
    }
    
    // 4. WiFi配置示例
    Serial.println("\n--- WiFi配置示例 ---");
    
    // 保存WiFi配置
    if (sdManager.saveWiFiConfig("TestWiFi", "test123456")) {
        Serial.println("WiFi配置保存成功");
    }
    
    // 读取WiFi配置
    String ssid, password;
    if (sdManager.loadWiFiConfig(ssid, password)) {
        Serial.println("WiFi SSID: " + ssid);
        Serial.println("WiFi密码: " + password);
    }
    
    // 5. 日志记录示例
    Serial.println("\n--- 日志记录示例 ---");
    
    // 写入系统日志
    sdManager.writeLog("SD卡示例程序开始运行");
    sdManager.writeLog("测试消息1: 系统正常");
    sdManager.writeLog("测试消息2: 传感器数据正常");
    
    // 写入自定义日志文件
    sdManager.writeLog("这是一条测试日志", "/test/test.log");
    
    // 读取日志
    String recentLogs = sdManager.readLog("/system.log", 5);
    if (!recentLogs.isEmpty()) {
        Serial.println("最近的系统日志:");
        Serial.println(recentLogs);
    }
    
    // 6. 目录列表示例
    Serial.println("\n--- 目录内容 ---");
    sdManager.listDir("/");
    sdManager.listDir("/config");
    sdManager.listDir("/test");
    
    // 7. 固件升级检查
    Serial.println("\n--- 固件升级检查 ---");
    if (sdManager.hasFirmwareUpdate()) {
        Serial.println("⚠️ 发现固件更新文件！");
        Serial.println("如需升级，请调用 sdManager.performFirmwareUpdate()");
    } else {
        Serial.println("✅ 没有发现固件更新文件");
    }
    
    // 8. 清理测试文件
    Serial.println("\n--- 清理测试文件 ---");
    sdManager.deleteFile("/test/config.json");
    sdManager.deleteFile("/test/test.log");
    
    Serial.println("\n=== SD卡示例完成 ===");
}

// 在main.cpp的setup()中调用此函数进行测试
void testSDCard() {
    delay(2000); // 等待串口稳定
    sdCardExample();
}

#endif // ENABLE_SDCARD
