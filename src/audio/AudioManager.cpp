#include "AudioManager.h"
#include <math.h>

static const char* TAG = "AudioManager";

AudioManager::AudioManager() : 
    initialized(false), 
    playing(false),
    wsPin(IIS_S_WS_PIN),
    bclkPin(IIS_S_BCLK_PIN), 
    dataPin(IIS_S_DATA_PIN) {
}

AudioManager::~AudioManager() {
    end();
}

bool AudioManager::begin(int ws_pin, int bclk_pin, int data_pin) {
    if (initialized) {
        ESP_LOGW(TAG, "AudioManager already initialized");
        return true;
    }
    
    wsPin = ws_pin;
    bclkPin = bclk_pin;
    dataPin = data_pin;
    
    ESP_LOGI(TAG, "Initializing AudioManager with pins: WS=%d, BCLK=%d, DATA=%d", 
             wsPin, bclkPin, dataPin);
    
    if (!initializeI2S()) {
        ESP_LOGE(TAG, "Failed to initialize I2S");
        return false;
    }
    
    initialized = true;
    ESP_LOGI(TAG, "AudioManager initialized successfully");
    return true;
}

void AudioManager::end() {
    if (!initialized) return;
    
    deinitializeI2S();
    initialized = false;
    playing = false;
    ESP_LOGI(TAG, "AudioManager deinitialized");
}

bool AudioManager::initializeI2S() {
    // I2S配置
    i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = AUDIO_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // 单声道，只使用左声道
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = AUDIO_BUFFER_SIZE,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    
    // 引脚配置
    pin_config = {
        .bck_io_num = bclkPin,
        .ws_io_num = wsPin,
        .data_out_num = dataPin,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    
    // 安装I2S驱动
    esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2S driver: %s", esp_err_to_name(err));
        return false;
    }
    
    // 设置引脚
    err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S pins: %s", esp_err_to_name(err));
        i2s_driver_uninstall(I2S_PORT);
        return false;
    }
    
    // 启动I2S
    err = i2s_start(I2S_PORT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start I2S: %s", esp_err_to_name(err));
        i2s_driver_uninstall(I2S_PORT);
        return false;
    }
    
    return true;
}

void AudioManager::deinitializeI2S() {
    i2s_stop(I2S_PORT);
    i2s_driver_uninstall(I2S_PORT);
}

bool AudioManager::playTone(float frequency, int duration_ms, float volume) {
    if (!initialized) {
        ESP_LOGE(TAG, "AudioManager not initialized");
        return false;
    }
    
    if (playing) {
        ESP_LOGW(TAG, "Audio already playing, skipping");
        return false;
    }
    
    playing = true;
    
    // 计算样本数
    int samples = (AUDIO_SAMPLE_RATE * duration_ms) / 1000;
    int16_t* audio_buffer = (int16_t*)malloc(samples * sizeof(int16_t));
    
    if (!audio_buffer) {
        ESP_LOGE(TAG, "Failed to allocate audio buffer");
        playing = false;
        return false;
    }
    
    // 生成正弦波
    float amplitude = 32767.0 * volume; // 16位最大值的音量比例
    for (int i = 0; i < samples; i++) {
        float t = (float)i / AUDIO_SAMPLE_RATE;
        audio_buffer[i] = (int16_t)(amplitude * sin(2.0 * M_PI * frequency * t));
    }
    
    // 播放音频
    size_t bytes_written = 0;
    esp_err_t err = i2s_write(I2S_PORT, audio_buffer, samples * sizeof(int16_t), 
                              &bytes_written, portMAX_DELAY);
    
    free(audio_buffer);
    playing = false;
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write I2S data: %s", esp_err_to_name(err));
        return false;
    }
    
    ESP_LOGI(TAG, "Played tone: %.1fHz for %dms, wrote %d bytes", 
             frequency, duration_ms, bytes_written);
    return true;
}

bool AudioManager::playBeepSequence(const float* frequencies, const int* durations, 
                                   int count, float volume) {
    if (!initialized) return false;
    
    for (int i = 0; i < count; i++) {
        if (!playTone(frequencies[i], durations[i], volume)) {
            return false;
        }
        // 短暂间隔
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    return true;
}

bool AudioManager::playBootSuccessSound() {
    ESP_LOGI(TAG, "Playing boot success sound");
    
    // 开机成功音：上升音调序列 (C-E-G-C)
    const float frequencies[] = {523.25, 659.25, 783.99, 1046.50}; // C5-E5-G5-C6
    const int durations[] = {200, 200, 200, 400};
    
    return playBeepSequence(frequencies, durations, 4, 0.6);
}

bool AudioManager::playWiFiConnectedSound() {
    ESP_LOGI(TAG, "Playing WiFi connected sound");
    
    // WiFi连接音：双音调
    const float frequencies[] = {800.0, 1200.0};
    const int durations[] = {150, 300};
    
    return playBeepSequence(frequencies, durations, 2, 0.5);
}

bool AudioManager::playGPSFixedSound() {
    ESP_LOGI(TAG, "Playing GPS fixed sound");
    
    // GPS定位音：三短音
    const float frequencies[] = {1000.0, 1000.0, 1000.0};
    const int durations[] = {100, 100, 100};
    
    return playBeepSequence(frequencies, durations, 3, 0.4);
}

bool AudioManager::playLowBatterySound() {
    ESP_LOGI(TAG, "Playing low battery sound");
    
    // 低电量警告音：下降音调
    const float frequencies[] = {1000.0, 800.0, 600.0};
    const int durations[] = {300, 300, 500};
    
    return playBeepSequence(frequencies, durations, 3, 0.7);
}

bool AudioManager::playSleepModeSound() {
    ESP_LOGI(TAG, "Playing sleep mode sound");
    
    // 休眠模式音：渐弱音调
    const float frequencies[] = {800.0, 600.0};
    const int durations[] = {200, 400};
    
    return playBeepSequence(frequencies, durations, 2, 0.3);
}

bool AudioManager::playCustomBeep(float frequency, int duration) {
    ESP_LOGI(TAG, "Playing custom beep: %.1fHz for %dms", frequency, duration);
    return playTone(frequency, duration, 0.5);
}

bool AudioManager::playAudioEvent(AudioEvent event) {
    switch (event) {
        case AUDIO_EVENT_BOOT_SUCCESS:
            return playBootSuccessSound();
        case AUDIO_EVENT_WIFI_CONNECTED:
            return playWiFiConnectedSound();
        case AUDIO_EVENT_GPS_FIXED:
            return playGPSFixedSound();
        case AUDIO_EVENT_LOW_BATTERY:
            return playLowBatterySound();
        case AUDIO_EVENT_SLEEP_MODE:
            return playSleepModeSound();
        case AUDIO_EVENT_CUSTOM:
        default:
            return playCustomBeep();
    }
}

bool AudioManager::setVolume(float volume) {
    // NS4168是数字功放，音量主要通过数字信号幅度控制
    // 这里记录音量设置，在播放时使用
    if (volume < 0.0) volume = 0.0;
    if (volume > 1.0) volume = 1.0;
    
    ESP_LOGI(TAG, "Volume set to %.2f", volume);
    return true;
}

bool AudioManager::testAudio() {
    if (!initialized) {
        ESP_LOGE(TAG, "AudioManager not initialized for test");
        return false;
    }
    
    ESP_LOGI(TAG, "Starting audio test sequence");
    
    // 测试序列：不同频率的音调
    const float test_frequencies[] = {440.0, 880.0, 1320.0, 660.0};
    const int test_durations[] = {300, 300, 300, 500};
    
    bool result = playBeepSequence(test_frequencies, test_durations, 4, 0.5);
    
    ESP_LOGI(TAG, "Audio test %s", result ? "PASSED" : "FAILED");
    return result;
}
