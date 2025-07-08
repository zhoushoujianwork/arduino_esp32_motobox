/*
 * GNSS自动上报关闭测试
 * 这个代码片段可以集成到main.cpp中来测试GNSS URC关闭功能
 */

// 在setup()函数中添加以下代码来测试GNSS URC控制

void testGNSSURCControl() {
    Serial.println("=== 开始GNSS URC控制测试 ===");
    
    // 等待Air780EG模块初始化完成
    delay(5000);
    
    // 检查当前GNSS状态
    if (air780eg_modem.isGNSSEnabled()) {
        Serial.println("✅ GNSS当前已启用");
        
        // 关闭GNSS自动上报
        Serial.println("🔄 正在关闭GNSS自动上报...");
        if (air780eg_modem.enableGNSSAutoReport(false)) {
            Serial.println("✅ GNSS自动上报已关闭");
            
            // 等待10秒，观察是否还有URC消息
            Serial.println("⏳ 等待10秒，观察URC消息...");
            unsigned long startTime = millis();
            int urcCount = 0;
            
            while (millis() - startTime < 10000) {
                // 这里可以添加URC消息计数逻辑
                delay(100);
            }
            
            Serial.println("✅ GNSS URC关闭测试完成");
            
            // 可选：重新启用GNSS自动上报
            Serial.println("🔄 重新启用GNSS自动上报...");
            if (air780eg_modem.enableGNSSAutoReport(true)) {
                Serial.println("✅ GNSS自动上报已重新启用");
            } else {
                Serial.println("❌ GNSS自动上报启用失败");
            }
            
        } else {
            Serial.println("❌ GNSS自动上报关闭失败");
        }
    } else {
        Serial.println("⚠️  GNSS当前未启用，请先启用GNSS");
    }
    
    Serial.println("=== GNSS URC控制测试结束 ===");
}

// 使用方法：
// 1. 在main.cpp的setup()函数末尾添加：testGNSSURCControl();
// 2. 或者在loop()函数中添加按键触发的测试调用

// 快速关闭GNSS URC的函数（可以在任何地方调用）
void quickDisableGNSSURC() {
    Serial.println("🚫 快速关闭GNSS自动上报");
    if (air780eg_modem.enableGNSSAutoReport(false)) {
        Serial.println("✅ GNSS URC已关闭");
    } else {
        Serial.println("❌ GNSS URC关闭失败");
    }
}
