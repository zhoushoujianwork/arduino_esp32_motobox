#ifndef WELCOME_AUDIO_H
#define WELCOME_AUDIO_H

#include <Arduino.h>

// 这是一个示例语音数据文件
// 实际使用时，需要将真实的"大菠萝车机,扎西德勒"语音文件转换为C数组

// 示例：简单的音调数据（实际应该是WAV音频数据）
// 这里用简单的正弦波数据作为占位符
const uint8_t welcome_voice_data[] = {
    // WAV文件头 (44字节)
    0x52, 0x49, 0x46, 0x46,  // "RIFF"
    0x24, 0x08, 0x00, 0x00,  // 文件大小-8
    0x57, 0x41, 0x56, 0x45,  // "WAVE"
    0x66, 0x6D, 0x74, 0x20,  // "fmt "
    0x10, 0x00, 0x00, 0x00,  // fmt chunk大小
    0x01, 0x00,              // 音频格式 (PCM)
    0x01, 0x00,              // 声道数 (1)
    0x80, 0x3E, 0x00, 0x00,  // 采样率 (16000)
    0x00, 0x7D, 0x00, 0x00,  // 字节率
    0x02, 0x00,              // 块对齐
    0x10, 0x00,              // 位深度 (16)
    0x64, 0x61, 0x74, 0x61,  // "data"
    0x00, 0x08, 0x00, 0x00,  // 数据大小
    
    // 音频数据 (这里是示例数据，实际应该是语音的PCM数据)
    // 以下是一个简单的音调序列，代表"大菠萝车机,扎西德勒"
    0x00, 0x00, 0x00, 0x10, 0x00, 0x20, 0x00, 0x30,
    0x00, 0x40, 0x00, 0x50, 0x00, 0x60, 0x00, 0x70,
    0x00, 0x80, 0x00, 0x90, 0x00, 0xA0, 0x00, 0xB0,
    0x00, 0xC0, 0x00, 0xD0, 0x00, 0xE0, 0x00, 0xF0,
    // ... 更多音频数据
};

const size_t welcome_voice_data_size = sizeof(welcome_voice_data);

// 语音文件信息
struct VoiceInfo {
    const char* name;
    const char* description;
    const uint8_t* data;
    size_t size;
    uint32_t sample_rate;
    uint8_t channels;
    uint8_t bits_per_sample;
};

// 预定义的语音信息
const VoiceInfo WELCOME_VOICE = {
    "welcome",
    "大菠萝车机,扎西德勒",
    welcome_voice_data,
    welcome_voice_data_size,
    16000,  // 16kHz
    1,      // 单声道
    16      // 16位
};

#endif // WELCOME_AUDIO_H
