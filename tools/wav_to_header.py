#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
WAV文件转C头文件工具
用于将语音文件转换为可以嵌入ESP32固件的C数组

使用方法:
python wav_to_header.py input.wav output.h variable_name "描述信息"

示例:
python wav_to_header.py welcome.wav welcome_audio.h welcome_voice_data "大菠萝车机,扎西德勒"
"""

import sys
import os
import wave
import struct
from datetime import datetime

def wav_to_header(input_file, output_file, var_name, description=""):
    """
    将WAV文件转换为C头文件
    
    Args:
        input_file: 输入的WAV文件路径
        output_file: 输出的C头文件路径
        var_name: C数组变量名
        description: 描述信息
    """
    
    if not os.path.exists(input_file):
        print(f"错误: 输入文件 {input_file} 不存在")
        return False
    
    try:
        # 读取WAV文件信息
        with wave.open(input_file, 'rb') as wav_file:
            frames = wav_file.getnframes()
            sample_rate = wav_file.getframerate()
            channels = wav_file.getnchannels()
            sample_width = wav_file.getsampwidth()
            
            print(f"WAV文件信息:")
            print(f"  采样率: {sample_rate} Hz")
            print(f"  声道数: {channels}")
            print(f"  位深度: {sample_width * 8} bit")
            print(f"  帧数: {frames}")
            print(f"  时长: {frames / sample_rate:.2f} 秒")
            
            # 检查格式是否符合要求
            if sample_rate != 16000:
                print(f"警告: 采样率为 {sample_rate} Hz，建议使用 16000 Hz")
            if channels != 1:
                print(f"警告: 声道数为 {channels}，建议使用单声道")
            if sample_width != 2:
                print(f"警告: 位深度为 {sample_width * 8} bit，建议使用 16 bit")
        
        # 读取整个文件的二进制数据
        with open(input_file, 'rb') as f:
            audio_data = f.read()
        
        file_size = len(audio_data)
        print(f"文件大小: {file_size} 字节")
        
        # 生成C头文件
        header_guard = f"{var_name.upper()}_H"
        
        header_content = f"""#ifndef {header_guard}
#define {header_guard}

#include <Arduino.h>

/*
 * 语音文件: {os.path.basename(input_file)}
 * 描述: {description}
 * 生成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
 * 
 * 音频参数:
 *   采样率: {sample_rate} Hz
 *   声道数: {channels}
 *   位深度: {sample_width * 8} bit
 *   时长: {frames / sample_rate:.2f} 秒
 *   文件大小: {file_size} 字节
 */

const uint8_t {var_name}[] = {{
"""
        
        # 将二进制数据转换为C数组格式
        for i, byte in enumerate(audio_data):
            if i % 16 == 0:
                header_content += "\n    "
            header_content += f"0x{byte:02X}"
            if i < len(audio_data) - 1:
                header_content += ", "
        
        header_content += f"""
}};

const size_t {var_name}_size = sizeof({var_name});

// 语音文件信息结构
struct VoiceInfo {{
    const char* name;
    const char* description;
    const uint8_t* data;
    size_t size;
    uint32_t sample_rate;
    uint8_t channels;
    uint8_t bits_per_sample;
}};

// 语音信息常量
const VoiceInfo {var_name.upper()}_INFO = {{
    "{var_name}",
    "{description}",
    {var_name},
    {var_name}_size,
    {sample_rate},
    {channels},
    {sample_width * 8}
}};

#endif // {header_guard}
"""
        
        # 写入头文件
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(header_content)
        
        print(f"成功生成头文件: {output_file}")
        print(f"数组变量名: {var_name}")
        print(f"数组大小: {var_name}_size")
        
        # 生成使用示例
        example_code = f"""
// 使用示例:
#include "{os.path.basename(output_file)}"

void playVoice() {{
    // 播放语音数据
    audioManager.playVoiceFromArray({var_name}, {var_name}_size);
    
    // 或者使用语音信息结构
    audioManager.playVoiceFromArray({var_name.upper()}_INFO.data, {var_name.upper()}_INFO.size);
}}
"""
        print(example_code)
        
        return True
        
    except Exception as e:
        print(f"错误: {e}")
        return False

def main():
    if len(sys.argv) < 4:
        print("用法: python wav_to_header.py <输入WAV文件> <输出头文件> <变量名> [描述]")
        print("")
        print("示例:")
        print('  python wav_to_header.py welcome.wav welcome_audio.h welcome_voice_data "大菠萝车机,扎西德勒"')
        print('  python wav_to_header.py boot.wav boot_audio.h boot_sound_data "开机音效"')
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    var_name = sys.argv[3]
    description = sys.argv[4] if len(sys.argv) > 4 else ""
    
    # 验证变量名
    if not var_name.replace('_', '').isalnum():
        print("错误: 变量名只能包含字母、数字和下划线")
        sys.exit(1)
    
    if var_name[0].isdigit():
        print("错误: 变量名不能以数字开头")
        sys.exit(1)
    
    success = wav_to_header(input_file, output_file, var_name, description)
    
    if success:
        print("\n转换完成！")
        print("\n下一步:")
        print(f"1. 将 {output_file} 复制到 src/audio/ 目录")
        print("2. 在 AudioManager.cpp 中包含头文件")
        print("3. 使用 playVoiceFromArray() 播放语音")
    else:
        print("\n转换失败！")
        sys.exit(1)

if __name__ == "__main__":
    main()
