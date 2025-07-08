/*
 * Air780EG Library Test Program
 * 
 * 这个测试程序用于验证Air780EG库是否正确集成到项目中
 * 可以用来替换现有的Air780EG相关代码进行测试
 */

#include <Arduino.h>
#include <Air780EG.h>

// 创建Air780EG实例
Air780EG air780;

// 串口配置 (根据你的硬件连接调整)
#define AIR780_SERIAL Serial1  // 根据你的项目配置调整
#define AIR780_BAUDRATE 115200

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== Air780EG Library Test ===");
    
    // 设置调试级别
    Air780EG::setLogLevel(AIR780EG_LOG_INFO);
    Air780EG::setLogOutput(&Serial);
    Air780EG::enableTimestamp(true);
    
    // 显示库版本
    Air780EG::printVersion();
    
    // 初始化Air780EG模块
    Serial.println("Initializing Air780EG library...");
    if (!air780.begin(&AIR780_SERIAL, AIR780_BAUDRATE)) {
        Serial.println("❌ Failed to initialize Air780EG library!");
        Serial.println("Please check:");
        Serial.println("- Serial connection");
        Serial.println("- Baudrate setting");
        Serial.println("- Module power supply");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("✅ Air780EG library initialized successfully!");
    
    // 测试网络功能
    Serial.println("\nTesting network functionality...");
    if (air780.getNetwork().enableNetwork()) {
        Serial.println("✅ Network enabled");
        air780.getNetwork().setUpdateInterval(5000); // 5秒更新间隔
    } else {
        Serial.println("❌ Failed to enable network");
    }
    
    // 测试GNSS功能
    Serial.println("\nTesting GNSS functionality...");
    if (air780.getGNSS().enableGNSS()) {
        Serial.println("✅ GNSS enabled");
        air780.getGNSS().setUpdateFrequency(1.0); // 1Hz更新频率
    } else {
        Serial.println("❌ Failed to enable GNSS");
    }
    
    Serial.println("\n=== Test Setup Complete ===");
    Serial.println("Monitoring status every 10 seconds...");
    Serial.println("Press Ctrl+C to stop\n");
}

void loop() {
    // 维护库的所有功能模块
    air780.loop();
    
    // 每10秒显示一次状态
    static unsigned long last_status_time = 0;
    if (millis() - last_status_time >= 10000) {
        last_status_time = millis();
        
        Serial.println("\n--- Status Update ---");
        
        // 测试网络状态获取
        if (air780.getNetwork().isDataValid()) {
            Serial.printf("📶 Network Status:\n");
            Serial.printf("  Registered: %s\n", 
                         air780.getNetwork().isNetworkRegistered() ? "✅ Yes" : "❌ No");
            Serial.printf("  Signal: %d dBm\n", 
                         air780.getNetwork().getSignalStrength());
            Serial.printf("  Operator: %s\n", 
                         air780.getNetwork().getOperatorName().c_str());
            Serial.printf("  Type: %s\n", 
                         air780.getNetwork().getNetworkType().c_str());
            Serial.printf("  IMEI: %s\n", 
                         air780.getNetwork().getIMEI().c_str());
        } else {
            Serial.println("📶 Network: Data not ready");
        }
        
        // 测试GNSS状态获取
        if (air780.getGNSS().isDataValid()) {
            Serial.printf("🛰️  GNSS Status:\n");
            Serial.printf("  Fixed: %s\n", 
                         air780.getGNSS().isFixed() ? "✅ Yes" : "❌ No");
            Serial.printf("  Satellites: %d\n", 
                         air780.getGNSS().getSatelliteCount());
            
            if (air780.getGNSS().isFixed()) {
                Serial.printf("  Location: %.6f, %.6f\n", 
                             air780.getGNSS().getLatitude(), 
                             air780.getGNSS().getLongitude());
                Serial.printf("  Altitude: %.2f m\n", 
                             air780.getGNSS().getAltitude());
                Serial.printf("  Speed: %.2f km/h\n", 
                             air780.getGNSS().getSpeed());
                Serial.printf("  Course: %.2f°\n", 
                             air780.getGNSS().getCourse());
            }
        } else {
            Serial.println("🛰️  GNSS: Data not ready");
        }
        
        Serial.printf("⏱️  Uptime: %lu seconds\n", millis() / 1000);
        Serial.println("-------------------");
    }
    
    // 每60秒显示详细状态
    static unsigned long last_detail_time = 0;
    if (millis() - last_detail_time >= 60000) {
        last_detail_time = millis();
        
        Serial.println("\n=== Detailed Status Report ===");
        air780.printStatus();
        Serial.println("==============================\n");
    }
    
    delay(100);
}

/*
 * 集成指南：
 * 
 * 1. 将现有的Air780EG相关代码替换为库调用
 * 2. 在main.cpp中包含 #include <Air780EG.h>
 * 3. 创建全局实例: Air780EG air780;
 * 4. 在setup()中调用 air780.begin()
 * 5. 在loop()中调用 air780.loop()
 * 6. 使用 air780.getNetwork() 和 air780.getGNSS() 获取功能模块
 * 
 * 优势：
 * - 减少AT指令调用次数
 * - 统一的接口和错误处理
 * - 完整的调试输出
 * - 可配置的更新频率
 * - 非阻塞的异步更新
 */
