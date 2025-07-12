#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <Arduino.h>
#include "driver/i2s.h"
#include "esp_log.h"

// 音频配置常量
#define AUDIO_SAMPLE_RATE       16000    // 采样率 16kHz
#define AUDIO_BITS_PER_SAMPLE   16       // 16位采样
#define AUDIO_CHANNELS          1        // 单声道
#define AUDIO_BUFFER_SIZE       1024     // 缓冲区大小

// I2S端口配置
#define I2S_PORT                I2S_NUM_0

// 音频事件类型
enum AudioEvent {
    AUDIO_EVENT_BOOT_SUCCESS,
    AUDIO_EVENT_WIFI_CONNECTED,
    AUDIO_EVENT_GPS_FIXED,
    AUDIO_EVENT_LOW_BATTERY,
    AUDIO_EVENT_SLEEP_MODE,
    AUDIO_EVENT_CUSTOM
};

// 欢迎语音类型
enum WelcomeVoiceType {
    WELCOME_VOICE_DEFAULT,      // 默认语音："大菠萝车机,扎西德勒"
    WELCOME_VOICE_LIFAN_MOTUO,  // 力帆摩托专用语音
    WELCOME_VOICE_DADADA,       // 新语音："大大大，大菠萝车机"
};

// 语音文件信息结构
struct VoiceInfo {
    const char* name;
    const char* description;
    const uint8_t* data;
    size_t size;
    uint32_t sample_rate;
    uint8_t channels;
    uint8_t bits_per_sample;
};

class AudioManager {
private:
    bool initialized;
    bool playing;
    int wsPin;      // WS (Word Select) 引脚
    int bclkPin;    // BCLK (Bit Clock) 引脚  
    int dataPin;    // DATA 引脚
    
    // 欢迎语音配置
    WelcomeVoiceType currentWelcomeVoice;
    
    // I2S配置结构体
    i2s_config_t i2s_config;
    i2s_pin_config_t pin_config;
    
    // 内部方法
    bool initializeI2S();
    void deinitializeI2S();
    bool playTone(float frequency, int duration_ms, float volume = 0.5);
    bool playBeepSequence(const float* frequencies, const int* durations, int count, float volume = 0.5);
    
public:
    AudioManager();
    ~AudioManager();
    
    // 初始化和配置
    bool begin(int ws_pin = IIS_S_WS_PIN, int bclk_pin = IIS_S_BCLK_PIN, int data_pin = IIS_S_DATA_PIN);
    void end();
    
    // 音频播放控制
    bool playBootSuccessSound();
    bool playWiFiConnectedSound();
    bool playGPSFixedSound();
    bool playLowBatterySound();
    bool playSleepModeSound();
    bool playCustomBeep(float frequency = 1000.0, int duration = 200);
    
    // 语音播放功能
    bool playWelcomeVoice();  // 播放当前配置的欢迎语音
    bool playWelcomeVoice(WelcomeVoiceType voiceType);  // 播放指定类型的欢迎语音
    bool playVoiceFromFile(const char* filename);
    bool playVoiceFromArray(const uint8_t* audioData, size_t dataSize);
    
    // 欢迎语音配置
    void setWelcomeVoiceType(WelcomeVoiceType voiceType);
    WelcomeVoiceType getWelcomeVoiceType() const;
    const char* getWelcomeVoiceDescription() const;
    
    // 音频事件播放
    bool playAudioEvent(AudioEvent event);
    
    // 状态查询
    bool isInitialized() const { return initialized; }
    bool isPlaying() const { return playing; }
    
    // 音量控制（通过PWM模拟）
    bool setVolume(float volume); // 0.0 - 1.0
    
    // 测试功能
    bool testAudio();

private:
    // 播放状态管理，防止重复播放
    struct PlaybackState {
        unsigned long lastBootSound = 0;
        unsigned long lastWiFiSound = 0;
        unsigned long lastGPSSound = 0;
        unsigned long lastBatterySound = 0;
        unsigned long lastSleepSound = 0;
        static const unsigned long MIN_INTERVAL = 3000; // 3秒内不重复播放同类音频
    };
    
    PlaybackState playbackState;
    
    // 检查是否可以播放指定类型的音频
    bool canPlaySound(unsigned long& lastPlayTime);
};

extern AudioManager audioManager;

#endif // AUDIOMANAGER_H
