#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
串口监控脚本 - 自动监控ESP32串口输出并保存到consoleout文件
"""

import serial
import time
import sys
import os
from datetime import datetime

def find_serial_port():
    """自动查找ESP32串口"""
    import glob
    
    # macOS常见的串口设备
    patterns = [
        '/dev/cu.usbserial-*',
        '/dev/cu.SLAB_USBtoUART*',
        '/dev/cu.wchusbserial*'
    ]
    
    ports = []
    for pattern in patterns:
        ports.extend(glob.glob(pattern))
    
    if ports:
        return sorted(ports)[-1]  # 返回最新的端口
    return None

def monitor_serial(port=None, baudrate=115200, output_file='consoleout'):
    """监控串口输出"""
    
    if not port:
        port = find_serial_port()
        if not port:
            print("❌ 未找到串口设备")
            print("请检查ESP32是否已连接")
            return
    
    print(f"🔌 连接串口: {port}")
    print(f"📊 波特率: {baudrate}")
    print(f"📝 输出文件: {output_file}")
    print("=" * 50)
    
    try:
        # 打开串口
        ser = serial.Serial(port, baudrate, timeout=1)
        time.sleep(2)  # 等待串口稳定
        
        # 打开输出文件
        with open(output_file, 'w', encoding='utf-8') as f:
            print(f"✅ 开始监控串口输出... (Ctrl+C 停止)")
            print(f"📁 输出保存到: {os.path.abspath(output_file)}")
            print("=" * 50)
            
            # 写入开始标记
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            header = f"=== 串口监控开始 {timestamp} ===\n"
            f.write(header)
            f.flush()
            
            line_count = 0
            
            while True:
                try:
                    # 读取串口数据
                    if ser.in_waiting > 0:
                        data = ser.readline()
                        try:
                            line = data.decode('utf-8', errors='ignore').rstrip()
                            if line:  # 只处理非空行
                                line_count += 1
                                
                                # 显示到控制台
                                print(f"[{line_count:04d}] {line}")
                                
                                # 写入文件
                                f.write(f"{line}\n")
                                f.flush()  # 立即刷新到文件
                                
                        except UnicodeDecodeError:
                            # 忽略解码错误
                            pass
                    
                    time.sleep(0.01)  # 短暂延时
                    
                except KeyboardInterrupt:
                    print("\n🛑 用户中断监控")
                    break
                except Exception as e:
                    print(f"❌ 读取错误: {e}")
                    time.sleep(1)
                    
    except serial.SerialException as e:
        print(f"❌ 串口错误: {e}")
        print("请检查:")
        print("1. ESP32是否已连接")
        print("2. 串口是否被其他程序占用")
        print("3. 串口权限是否正确")
        
    except Exception as e:
        print(f"❌ 未知错误: {e}")
        
    finally:
        try:
            ser.close()
            print("🔌 串口已关闭")
        except:
            pass

def main():
    """主函数"""
    print("🚀 ESP32串口监控工具")
    print("=" * 50)
    
    # 解析命令行参数
    port = None
    baudrate = 115200
    output_file = 'consoleout'
    
    if len(sys.argv) > 1:
        port = sys.argv[1]
    if len(sys.argv) > 2:
        try:
            baudrate = int(sys.argv[2])
        except ValueError:
            print("❌ 波特率必须是数字")
            return
    if len(sys.argv) > 3:
        output_file = sys.argv[3]
    
    # 开始监控
    monitor_serial(port, baudrate, output_file)

if __name__ == "__main__":
    main()
