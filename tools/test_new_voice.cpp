/*
 * 测试新语音功能的示例代码
 * 
 * 这个文件展示了如何在主程序中使用新的语音功能
 * 可以将相关代码片段复制到 main.cpp 中使用
 */

#include "src/audio/AudioManager.h"

// 全局音频管理器实例
AudioManager audioManager;

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32-S3 MotoBox 新语音测试");
    
    // 初始化音频管理器
    if (!audioManager.begin()) {
        Serial.println("❌ 音频管理器初始化失败");
        return;
    }
    
    Serial.println("✅ 音频管理器初始化成功");
    
    // 测试播放默认语音
    Serial.println("播放默认语音：大菠萝车机,扎西德勒");
    audioManager.setWelcomeVoiceType(WELCOME_VOICE_DEFAULT);
    audioManager.playWelcomeVoice();
    
    delay(4000); // 等待播放完成
    
    // 测试播放新语音
    Serial.println("播放新语音：大大大，大菠萝车机");
    audioManager.setWelcomeVoiceType(WELCOME_VOICE_DADADA);
    audioManager.playWelcomeVoice();
    
    delay(4000); // 等待播放完成
    
    // 显示当前语音信息
    Serial.printf("当前语音类型: %s\n", audioManager.getWelcomeVoiceDescription());
}

void loop() {
    // 每10秒切换一次语音类型并播放
    static unsigned long lastSwitch = 0;
    static bool useNewVoice = false;
    
    if (millis() - lastSwitch > 10000) {
        useNewVoice = !useNewVoice;
        
        if (useNewVoice) {
            Serial.println("切换到新语音：大大大，大菠萝车机");
            audioManager.setWelcomeVoiceType(WELCOME_VOICE_DADADA);
        } else {
            Serial.println("切换到默认语音：大菠萝车机,扎西德勒");
            audioManager.setWelcomeVoiceType(WELCOME_VOICE_DEFAULT);
        }
        
        audioManager.playWelcomeVoice();
        lastSwitch = millis();
    }
    
    delay(100);
}

/*
 * 在主程序中的使用方法：
 * 
 * 1. 在 main.cpp 的全局变量区域添加：
 *    extern AudioManager audioManager;
 * 
 * 2. 在需要播放新语音的地方调用：
 *    audioManager.setWelcomeVoiceType(WELCOME_VOICE_DADADA);
 *    audioManager.playWelcomeVoice();
 * 
 * 3. 或者直接播放指定类型的语音：
 *    audioManager.playWelcomeVoice(WELCOME_VOICE_DADADA);
 * 
 * 4. 查询当前语音信息：
 *    const char* desc = audioManager.getWelcomeVoiceDescription();
 *    Serial.printf("当前语音: %s\n", desc);
 */
