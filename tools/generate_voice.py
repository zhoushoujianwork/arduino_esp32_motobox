#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
语音生成工具
使用多种TTS服务生成"大菠萝车机,扎西德勒"语音文件
"""

import os
import sys
import requests
import json
import base64
import hashlib
import time
from urllib.parse import urlencode
import wave
import subprocess

def generate_with_edge_tts(text, output_file):
    """
    使用Edge TTS生成语音（免费，推荐）
    需要安装: pip install edge-tts
    """
    try:
        import edge_tts
        import asyncio
        
        async def generate():
            # 使用中文女声
            voice = "zh-CN-XiaoxiaoNeural"  # 温柔女声
            # voice = "zh-CN-YunxiNeural"   # 男声备选
            
            communicate = edge_tts.Communicate(text, voice)
            await communicate.save(output_file)
            
        asyncio.run(generate())
        print(f"✅ Edge TTS生成成功: {output_file}")
        return True
        
    except ImportError:
        print("❌ Edge TTS未安装，请运行: pip install edge-tts")
        return False
    except Exception as e:
        print(f"❌ Edge TTS生成失败: {e}")
        return False

def generate_with_gtts(text, output_file):
    """
    使用Google TTS生成语音
    需要安装: pip install gtts
    """
    try:
        from gtts import gTTS
        
        # 生成中文语音
        tts = gTTS(text=text, lang='zh', slow=False)
        temp_file = output_file.replace('.wav', '_temp.mp3')
        tts.save(temp_file)
        
        # 转换为WAV格式
        convert_to_wav(temp_file, output_file)
        os.remove(temp_file)
        
        print(f"✅ Google TTS生成成功: {output_file}")
        return True
        
    except ImportError:
        print("❌ Google TTS未安装，请运行: pip install gtts")
        return False
    except Exception as e:
        print(f"❌ Google TTS生成失败: {e}")
        return False

def generate_with_baidu_tts(text, output_file, api_key=None, secret_key=None):
    """
    使用百度TTS生成语音
    需要百度AI开放平台的API Key
    """
    if not api_key or not secret_key:
        print("❌ 百度TTS需要API Key和Secret Key")
        return False
    
    try:
        # 获取access_token
        token_url = "https://aip.baidubce.com/oauth/2.0/token"
        token_params = {
            'grant_type': 'client_credentials',
            'client_id': api_key,
            'client_secret': secret_key
        }
        
        token_response = requests.get(token_url, params=token_params)
        access_token = token_response.json().get('access_token')
        
        if not access_token:
            print("❌ 获取百度access_token失败")
            return False
        
        # 生成语音
        tts_url = "https://tsn.baidu.com/text2audio"
        tts_params = {
            'tex': text,
            'tok': access_token,
            'cuid': 'esp32_motobox',
            'ctp': 1,
            'lan': 'zh',
            'spd': 5,    # 语速 1-15
            'pit': 5,    # 音调 1-15  
            'vol': 8,    # 音量 1-15
            'per': 1,    # 发音人 1-4
            'aue': 6     # 音频格式 6=wav
        }
        
        response = requests.get(tts_url, params=tts_params)
        
        if response.status_code == 200:
            with open(output_file, 'wb') as f:
                f.write(response.content)
            print(f"✅ 百度TTS生成成功: {output_file}")
            return True
        else:
            print(f"❌ 百度TTS请求失败: {response.status_code}")
            return False
            
    except Exception as e:
        print(f"❌ 百度TTS生成失败: {e}")
        return False

def convert_to_wav(input_file, output_file, sample_rate=16000):
    """
    使用FFmpeg转换音频格式为ESP32兼容的WAV格式
    """
    try:
        cmd = [
            'ffmpeg', '-i', input_file,
            '-ar', str(sample_rate),  # 采样率16kHz
            '-ac', '1',               # 单声道
            '-sample_fmt', 's16',     # 16位PCM
            '-y',                     # 覆盖输出文件
            output_file
        ]
        
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode == 0:
            print(f"✅ 音频转换成功: {output_file}")
            return True
        else:
            print(f"❌ FFmpeg转换失败: {result.stderr}")
            return False
            
    except FileNotFoundError:
        print("❌ FFmpeg未安装，请安装FFmpeg")
        return False
    except Exception as e:
        print(f"❌ 音频转换失败: {e}")
        return False

def validate_wav_format(wav_file):
    """
    验证WAV文件格式是否符合ESP32要求
    """
    try:
        with wave.open(wav_file, 'rb') as wav:
            frames = wav.getnframes()
            sample_rate = wav.getframerate()
            channels = wav.getnchannels()
            sample_width = wav.getsampwidth()
            
            print(f"\n📊 音频文件信息:")
            print(f"   文件: {wav_file}")
            print(f"   采样率: {sample_rate} Hz")
            print(f"   声道数: {channels}")
            print(f"   位深度: {sample_width * 8} bit")
            print(f"   时长: {frames / sample_rate:.2f} 秒")
            print(f"   文件大小: {os.path.getsize(wav_file)} 字节")
            
            # 检查格式
            issues = []
            if sample_rate != 16000:
                issues.append(f"采样率应为16000Hz，当前为{sample_rate}Hz")
            if channels != 1:
                issues.append(f"应为单声道，当前为{channels}声道")
            if sample_width != 2:
                issues.append(f"应为16位，当前为{sample_width * 8}位")
            
            if issues:
                print("⚠️  格式问题:")
                for issue in issues:
                    print(f"   - {issue}")
                return False
            else:
                print("✅ 格式符合ESP32要求")
                return True
                
    except Exception as e:
        print(f"❌ 验证WAV文件失败: {e}")
        return False

def main():
    text = "大菠萝车机，扎西德勒"
    output_dir = "audio_files"
    
    # 创建输出目录
    os.makedirs(output_dir, exist_ok=True)
    
    print("🎵 开始生成语音文件...")
    print(f"📝 文本内容: {text}")
    print(f"📁 输出目录: {output_dir}")
    print("-" * 50)
    
    success_files = []
    
    # 方法1: Edge TTS (推荐，免费)
    print("\n1️⃣ 尝试使用Edge TTS...")
    edge_file = os.path.join(output_dir, "welcome_edge.wav")
    if generate_with_edge_tts(text, edge_file):
        if validate_wav_format(edge_file):
            success_files.append(("Edge TTS", edge_file))
    
    # 方法2: Google TTS
    print("\n2️⃣ 尝试使用Google TTS...")
    gtts_file = os.path.join(output_dir, "welcome_gtts.wav")
    if generate_with_gtts(text, gtts_file):
        if validate_wav_format(gtts_file):
            success_files.append(("Google TTS", gtts_file))
    
    # 方法3: 百度TTS (需要API Key)
    print("\n3️⃣ 百度TTS需要API Key，跳过...")
    
    # 总结结果
    print("\n" + "="*50)
    print("🎉 语音生成完成!")
    
    if success_files:
        print(f"✅ 成功生成 {len(success_files)} 个语音文件:")
        for method, file_path in success_files:
            print(f"   - {method}: {file_path}")
        
        # 选择最佳文件
        best_file = success_files[0][1]  # 使用第一个成功的文件
        final_file = os.path.join(output_dir, "welcome.wav")
        
        if best_file != final_file:
            import shutil
            shutil.copy2(best_file, final_file)
            print(f"\n📋 最佳文件已复制为: {final_file}")
        
        # 转换为C头文件
        print("\n🔄 开始转换为C头文件...")
        header_file = os.path.join(output_dir, "welcome_voice.h")
        
        # 调用转换工具
        convert_cmd = [
            sys.executable, 
            "wav_to_header.py",
            final_file,
            header_file,
            "welcome_voice_data",
            "大菠萝车机,扎西德勒"
        ]
        
        try:
            result = subprocess.run(convert_cmd, cwd=os.path.dirname(__file__), 
                                  capture_output=True, text=True)
            if result.returncode == 0:
                print("✅ C头文件生成成功!")
                print(f"📄 头文件: {header_file}")
                print("\n📋 下一步操作:")
                print(f"1. 将 {final_file} 复制到 data/ 目录")
                print(f"2. 将 {header_file} 复制到 src/audio/ 目录")
                print("3. 运行 'pio run --target uploadfs' 上传文件")
                print("4. 编译并烧录固件")
            else:
                print(f"❌ C头文件生成失败: {result.stderr}")
        except Exception as e:
            print(f"❌ 调用转换工具失败: {e}")
    
    else:
        print("❌ 没有成功生成任何语音文件")
        print("💡 建议:")
        print("   1. 安装Edge TTS: pip install edge-tts")
        print("   2. 安装Google TTS: pip install gtts")
        print("   3. 安装FFmpeg用于音频转换")

if __name__ == "__main__":
    main()
