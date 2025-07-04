/*
 * 音频系统测试代码
 * 用于独立测试音频功能
 */

#include <Arduino.h>
#include "audio/AudioManager.h"

AudioManager audioManager;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== 音频系统测试 ===");
    
    // 检查音频引脚定义
#ifdef IIS_S_WS_PIN
    Serial.printf("音频引脚配置:\n");
    Serial.printf("  WS引脚: %d\n", IIS_S_WS_PIN);
    Serial.printf("  BCLK引脚: %d\n", IIS_S_BCLK_PIN);
    Serial.printf("  DATA引脚: %d\n", IIS_S_DATA_PIN);
#else
    Serial.println("❌ 音频引脚未定义");
    return;
#endif

    // 初始化音频系统
    Serial.println("正在初始化音频系统...");
    if (audioManager.begin()) {
        Serial.println("✅ 音频系统初始化成功");
        
        // 播放测试音频
        Serial.println("播放测试音频...");
        audioManager.playBootSuccessSound();
        
        delay(2000);
        
        Serial.println("播放自定义音频...");
        audioManager.playCustomBeep(1000, 500);
        
    } else {
        Serial.println("❌ 音频系统初始化失败");
    }
}

void loop() {
    static unsigned long lastTest = 0;
    
    if (millis() - lastTest > 10000) {  // 每10秒测试一次
        lastTest = millis();
        
        Serial.println("播放测试音频...");
        audioManager.playCustomBeep(800, 200);
        delay(300);
        audioManager.playCustomBeep(1200, 200);
    }
    
    delay(100);
}
