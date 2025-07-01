/*
 * SD卡快速修复和诊断示例
 * 用于快速诊断和解决SD卡初始化问题
 */

#include "SDManager.h"

// 快速修复函数
void quickFixSDCard() {
    Serial.println("\n=== SD卡快速修复程序 ===");
    
    // 步骤1：基础硬件检查
    Serial.println("步骤1: 基础硬件检查");
    Serial.println("请确认以下硬件连接:");
    Serial.printf("  CS   (片选)   → GPIO%d\n", SD_CS_PIN);
    Serial.printf("  MOSI (数据出) → GPIO%d\n", SD_MOSI_PIN);
    Serial.printf("  MISO (数据入) → GPIO%d\n", SD_MISO_PIN);
    Serial.printf("  SCK  (时钟)   → GPIO%d\n", SD_SCK_PIN);
    Serial.println("  VCC  (电源)   → 3.3V");
    Serial.println("  GND  (接地)   → GND");
    
    delay(2000);
    
    // 步骤2：引脚状态检查
    Serial.println("\n步骤2: 引脚状态检查");
    pinMode(SD_CS_PIN, INPUT_PULLUP);
    pinMode(SD_MISO_PIN, INPUT_PULLUP);
    delay(10);
    
    Serial.printf("  CS引脚状态:   %s\n", digitalRead(SD_CS_PIN) ? "HIGH (正常)" : "LOW (可能短路)");
    Serial.printf("  MISO引脚状态: %s\n", digitalRead(SD_MISO_PIN) ? "HIGH (正常)" : "LOW (可能无连接)");
    
    // 步骤3：尝试不同频率初始化
    Serial.println("\n步骤3: 尝试不同频率初始化");
    
    const uint32_t frequencies[] = {400000, 1000000, 4000000, 8000000};
    const char* freqNames[] = {"400kHz", "1MHz", "4MHz", "8MHz"};
    bool initSuccess = false;
    
    for (int i = 0; i < 4; i++) {
        Serial.printf("  尝试 %s 频率...", freqNames[i]);
        
        // 重置SPI
        SD.end();
        delay(100);
        SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
        
        // 设置CS引脚
        pinMode(SD_CS_PIN, OUTPUT);
        digitalWrite(SD_CS_PIN, HIGH);
        delay(10);
        
        if (SD.begin(SD_CS_PIN, SPI, frequencies[i])) {
            Serial.println(" ✅ 成功!");
            initSuccess = true;
            
            // 显示SD卡信息
            Serial.println("\n  SD卡信息:");
            Serial.printf("    类型: %s\n", SD.cardType() == CARD_MMC ? "MMC" : 
                         SD.cardType() == CARD_SD ? "SDSC" :
                         SD.cardType() == CARD_SDHC ? "SDHC" : "未知");
            Serial.printf("    大小: %.2f GB\n", SD.cardSize() / (1024.0 * 1024.0 * 1024.0));
            Serial.printf("    扇区大小: %d bytes\n", SD.sectorSize());
            break;
        } else {
            Serial.println(" ❌ 失败");
        }
    }
    
    if (!initSuccess) {
        Serial.println("\n❌ 所有频率都初始化失败");
        Serial.println("\n可能的问题:");
        Serial.println("  1. SD卡未正确插入");
        Serial.println("  2. 硬件连接错误");
        Serial.println("  3. SD卡已损坏");
        Serial.println("  4. 电源供电不足");
        Serial.println("  5. 引脚配置冲突");
        
        // 执行详细诊断
        performDetailedDiagnostic();
        return;
    }
    
    // 步骤4：基本功能测试
    Serial.println("\n步骤4: 基本功能测试");
    
    // 测试文件写入
    Serial.print("  测试文件写入...");
    File testFile = SD.open("/test.txt", FILE_WRITE);
    if (testFile) {
        testFile.println("SD Card Test - " + String(millis()));
        testFile.close();
        Serial.println(" ✅ 成功");
    } else {
        Serial.println(" ❌ 失败");
        return;
    }
    
    // 测试文件读取
    Serial.print("  测试文件读取...");
    testFile = SD.open("/test.txt", FILE_READ);
    if (testFile) {
        String content = testFile.readString();
        testFile.close();
        if (content.length() > 0) {
            Serial.println(" ✅ 成功");
            Serial.println("    内容: " + content.substring(0, content.indexOf('\n')));
        } else {
            Serial.println(" ❌ 读取内容为空");
        }
    } else {
        Serial.println(" ❌ 无法打开文件");
        return;
    }
    
    // 测试文件删除
    Serial.print("  测试文件删除...");
    if (SD.remove("/test.txt")) {
        Serial.println(" ✅ 成功");
    } else {
        Serial.println(" ❌ 失败");
    }
    
    Serial.println("\n✅ SD卡快速修复完成！SD卡工作正常。");
}

// 详细诊断函数
void performDetailedDiagnostic() {
    Serial.println("\n=== 详细硬件诊断 ===");
    
    // 检查引脚配置
    Serial.println("引脚配置检查:");
    int pins[] = {SD_CS_PIN, SD_MOSI_PIN, SD_MISO_PIN, SD_SCK_PIN};
    String pinNames[] = {"CS", "MOSI", "MISO", "SCK"};
    
    for (int i = 0; i < 4; i++) {
        Serial.printf("  %s (GPIO%d): ", pinNames[i].c_str(), pins[i]);
        
        // 检查引脚是否在有效范围
        if (pins[i] < 0 || pins[i] > 48) {
            Serial.println("❌ 引脚超出范围");
            continue;
        }
        
        // 检查引脚是否为输入专用引脚
        if (pins[i] >= 34 && pins[i] <= 39 && (i == 0 || i == 1)) { // CS和MOSI不能是输入专用
            Serial.println("⚠️  输入专用引脚，不适合作为输出");
            continue;
        }
        
        Serial.println("✅ 配置正常");
    }
    
    // 检查引脚冲突
    Serial.println("\n引脚冲突检查:");
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (pins[i] == pins[j]) {
                Serial.printf("  ❌ %s 和 %s 使用相同引脚 GPIO%d\n", 
                             pinNames[i].c_str(), pinNames[j].c_str(), pins[i]);
            }
        }
    }
    
    // SPI通信测试
    Serial.println("\nSPI通信测试:");
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    delay(10);
    
    // 发送空闲命令
    digitalWrite(SD_CS_PIN, LOW);
    delay(1);
    
    uint8_t response = 0xFF;
    for (int i = 0; i < 10; i++) {
        response = SPI.transfer(0xFF);
        if (response != 0xFF) break;
        delay(1);
    }
    
    digitalWrite(SD_CS_PIN, HIGH);
    
    if (response == 0xFF) {
        Serial.println("  ❌ 无SPI响应");
        Serial.println("     建议检查:");
        Serial.println("     - SD卡是否正确插入");
        Serial.println("     - 引脚连接是否正确");
        Serial.println("     - SD卡是否损坏");
    } else {
        Serial.printf("  ✅ 收到响应: 0x%02X\n", response);
    }
    
    // 电源建议
    Serial.println("\n电源检查建议:");
    Serial.println("  - 使用万用表测量SD卡VCC引脚电压");
    Serial.println("  - 应为3.3V ± 0.1V");
    Serial.println("  - 检查GND连接是否良好");
    Serial.println("  - 确保ESP32供电充足");
    
    Serial.println("\n=== 诊断完成 ===");
}

// 引脚配置建议
void suggestPinConfiguration() {
    Serial.println("\n=== 引脚配置建议 ===");
    
    Serial.println("当前配置:");
    Serial.printf("  CS=%d, MOSI=%d, MISO=%d, SCK=%d\n", 
                  SD_CS_PIN, SD_MOSI_PIN, SD_MISO_PIN, SD_SCK_PIN);
    
    Serial.println("\n推荐配置方案:");
    
    Serial.println("方案1 (适用于大多数ESP32开发板):");
    Serial.println("  #define SD_CS_PIN    5");
    Serial.println("  #define SD_MOSI_PIN  23");
    Serial.println("  #define SD_MISO_PIN  19");
    Serial.println("  #define SD_SCK_PIN   18");
    
    Serial.println("\n方案2 (避免与WiFi冲突):");
    Serial.println("  #define SD_CS_PIN    15");
    Serial.println("  #define SD_MOSI_PIN  13");
    Serial.println("  #define SD_MISO_PIN  12");
    Serial.println("  #define SD_SCK_PIN   14");
    
    Serial.println("\n方案3 (使用VSPI):");
    Serial.println("  #define SD_CS_PIN    5");
    Serial.println("  #define SD_MOSI_PIN  23");
    Serial.println("  #define SD_MISO_PIN  19");
    Serial.println("  #define SD_SCK_PIN   18");
    
    Serial.println("\n注意事项:");
    Serial.println("  - 避免使用GPIO 0, 1, 3 (启动相关)");
    Serial.println("  - 避免使用GPIO 34-39 作为输出引脚");
    Serial.println("  - 确保引脚不与其他功能冲突");
}

// 使用示例
void setupSDQuickFix() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("SD卡问题诊断工具");
    Serial.println("输入命令:");
    Serial.println("  'fix' - 执行快速修复");
    Serial.println("  'diag' - 详细诊断");
    Serial.println("  'pins' - 引脚配置建议");
}

void loopSDQuickFix() {
    if (Serial.available()) {
        String command = Serial.readString();
        command.trim();
        command.toLowerCase();
        
        if (command == "fix") {
            quickFixSDCard();
        } else if (command == "diag") {
            performDetailedDiagnostic();
        } else if (command == "pins") {
            suggestPinConfiguration();
        } else {
            Serial.println("未知命令，请输入 'fix', 'diag' 或 'pins'");
        }
    }
}
