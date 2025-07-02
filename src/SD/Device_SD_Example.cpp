/*
 * SDManager高级功能使用示例
 * 展示如何使用SDManager中的格式化、健康检查和修复功能
 */

#ifdef ENABLE_SDCARD
#include "SD/SDManager.h"

void demonstrateSDCardManagement() {
    Serial.println("=== SDManager高级功能演示 ===");
    
    // 启用调试输出
    sdManager.setDebug(true);
    
    // 确保SD卡已初始化
    if (!sdManager.isInitialized()) {
        Serial.println("初始化SD卡...");
        if (!sdManager.begin()) {
            Serial.println("❌ SD卡初始化失败，无法继续演示");
            return;
        }
    }
    
    // 1. SD卡健康检查
    Serial.println("\n--- 步骤1: SD卡健康检查 ---");
    bool isHealthy = sdManager.checkSDCardHealth();
    
    if (isHealthy) {
        Serial.println("✅ SD卡状态良好");
    } else {
        Serial.println("⚠️  SD卡存在问题，建议修复");
        
        // 2. 尝试修复SD卡
        Serial.println("\n--- 步骤2: SD卡修复 ---");
        sdManager.repairSDCard();
        
        // 再次检查健康状态
        Serial.println("\n--- 步骤3: 修复后健康检查 ---");
        isHealthy = sdManager.checkSDCardHealth();
        
        if (isHealthy) {
            Serial.println("✅ SD卡修复成功");
        } else {
            Serial.println("❌ SD卡修复失败，可能需要格式化");
            
            // 3. 格式化SD卡（谨慎操作）
            Serial.println("\n--- 步骤4: SD卡格式化 ---");
            Serial.println("⚠️  警告：这将删除SD卡上的所有数据！");
            Serial.println("在实际使用中，请确认用户同意后再执行格式化");
            
            // 在实际应用中，这里应该有用户确认机制
            // 例如：按钮确认、串口输入确认等
            
            // 演示目的，这里注释掉实际的格式化调用
            /*
            if (sdManager.formatSDCard()) {
                Serial.println("✅ SD卡格式化成功");
                
                // 格式化后再次检查
                if (sdManager.checkSDCardHealth()) {
                    Serial.println("✅ 格式化后SD卡状态良好");
                }
            } else {
                Serial.println("❌ SD卡格式化失败");
            }
            */
            
            Serial.println("💡 格式化功能已准备就绪，如需使用请取消注释相关代码");
        }
    }
    
    // 4. 显示SD卡信息
    Serial.println("\n--- SD卡当前状态 ---");
    sdManager.printCardInfo();
    sdManager.listDir("/");
    
    Serial.println("\n=== SD卡管理功能演示完成 ===");
}

// 串口命令处理示例
void handleSDCardCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        
        // 使用SDManager的命令处理器
        if (!sdManager.handleSerialCommand(command)) {
            // 如果SDManager没有处理这个命令，可以处理其他命令
            command.trim();
            command.toLowerCase();
            
            if (command == "help" || command == "h") {
                Serial.println("可用命令:");
                sdManager.showHelp();
                Serial.println("  help      (h)   - 显示此帮助");
            }
        }
    }
}

// 在main.cpp的loop()中调用此函数
void processSDCardCommands() {
    handleSDCardCommands();
}

// 在main.cpp的setup()中调用此函数进行演示
void runSDCardManagementDemo() {
    delay(3000); // 等待系统稳定
    demonstrateSDCardManagement();
    
    Serial.println("\n💡 提示：可以通过串口发送命令来管理SD卡");
    Serial.println("发送 'sd_help' 查看SD卡管理命令");
    Serial.println("发送 'help' 查看所有命令");
}

#endif // ENABLE_SDCARD
