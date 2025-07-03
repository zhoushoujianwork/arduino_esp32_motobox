/*
 * Air780EG 初始化测试程序
 * 用于验证Air780EG模块的基本初始化和GSM_EN引脚控制
 */

#include "Air780EGModem.h"

void air780eg_init_test() {
    Serial.println("=== Air780EG 初始化测试 ===");
    
    // 显示引脚配置
    Serial.printf("GSM_RX_PIN: %d\n", GSM_RX_PIN);
    Serial.printf("GSM_TX_PIN: %d\n", GSM_TX_PIN);
    Serial.printf("GSM_EN: %d\n", GSM_EN);
    
    // 手动控制GSM_EN引脚
    Serial.println("手动控制GSM_EN引脚...");
    pinMode(GSM_EN, OUTPUT);
    
    // 先拉低，然后拉高
    Serial.println("GSM_EN -> LOW");
    digitalWrite(GSM_EN, LOW);
    delay(1000);
    
    Serial.println("GSM_EN -> HIGH");
    digitalWrite(GSM_EN, HIGH);
    delay(2000);
    
    // 检查引脚状态
    Serial.printf("GSM_EN 当前状态: %s\n", digitalRead(GSM_EN) ? "HIGH" : "LOW");
    
    // 初始化Air780EG模块
    Serial.println("初始化Air780EG模块...");
    air780eg_modem.setDebug(true);
    
    if (air780eg_modem.begin()) {
        Serial.println("✅ Air780EG初始化成功");
        
        // 基础AT测试
        Serial.println("测试基础AT命令...");
        if (air780eg_modem.sendAT("AT", "OK", 2000)) {
            Serial.println("✅ AT命令响应正常");
        } else {
            Serial.println("❌ AT命令无响应");
        }
        
        // 获取模块信息
        String moduleName = air780eg_modem.getModuleName();
        String imei = air780eg_modem.getIMEI();
        
        Serial.println("模块信息:");
        Serial.println("  名称: " + moduleName);
        Serial.println("  IMEI: " + imei);
        
        // 检查网络状态
        Serial.println("检查网络状态...");
        if (air780eg_modem.isNetworkReadyCheck()) {
            Serial.println("✅ 网络已注册");
            air780eg_modem.debugNetworkInfo();
        } else {
            Serial.println("⚠️ 网络未注册，等待中...");
        }
        
    } else {
        Serial.println("❌ Air780EG初始化失败");
        
        // 调试信息
        Serial.println("调试信息:");
        Serial.printf("  串口: Serial2\n");
        Serial.printf("  波特率: 115200\n");
        Serial.printf("  RX引脚: %d\n", GSM_RX_PIN);
        Serial.printf("  TX引脚: %d\n", GSM_TX_PIN);
        Serial.printf("  EN引脚: %d (状态: %s)\n", GSM_EN, digitalRead(GSM_EN) ? "HIGH" : "LOW");
    }
    
    Serial.println("=== 测试完成 ===");
}

void air780eg_continuous_test() {
    static unsigned long lastTest = 0;
    
    if (millis() - lastTest > 10000) {  // 每10秒测试一次
        lastTest = millis();
        
        Serial.println("\n--- Air780EG 状态检查 ---");
        
        // AT命令测试
        if (air780eg_modem.sendAT("AT", "OK", 1000)) {
            Serial.println("AT: OK");
        } else {
            Serial.println("AT: 无响应");
        }
        
        // 网络状态
        if (air780eg_modem.isNetworkReadyCheck()) {
            Serial.println("网络: 已注册");
            Serial.println("信号强度: " + String(air780eg_modem.getCSQ()));
        } else {
            Serial.println("网络: 未注册");
        }
        
        // GNSS状态
        if (air780eg_modem.isGNSSEnabled()) {
            Serial.println("GNSS: 已启用");
            if (air780eg_modem.updateGNSSData()) {
                GNSSData gnss = air780eg_modem.getGNSSData();
                if (gnss.valid) {
                    Serial.printf("位置: %.6f, %.6f\n", gnss.latitude, gnss.longitude);
                } else {
                    Serial.println("GNSS: 无有效定位");
                }
            }
        } else {
            Serial.println("GNSS: 未启用");
        }
    }
}
