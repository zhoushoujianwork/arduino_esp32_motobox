#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ESP32串口监控脚本 - 增强版
功能：
- 自动监控ESP32串口输出并保存到consoleout目录
- 按日期和序号组织输出文件
- 支持实时显示和文件保存
- 支持过滤和高亮显示
- 支持统计信息显示
"""

import serial
import time
import sys
import os
import glob
from datetime import datetime
import re
import argparse

class SerialMonitor:
    def __init__(self, port=None, baudrate=115200, output_dir='consoleout'):
        self.port = port
        self.baudrate = baudrate
        self.output_dir = output_dir
        self.ser = None
        self.output_file = None
        self.line_count = 0
        self.error_count = 0
        self.mqtt_count = 0
        self.start_time = None
        
        # 确保输出目录存在
        os.makedirs(output_dir, exist_ok=True)
        
        # 颜色代码
        self.colors = {
            'red': '\033[91m',
            'green': '\033[92m',
            'yellow': '\033[93m',
            'blue': '\033[94m',
            'magenta': '\033[95m',
            'cyan': '\033[96m',
            'white': '\033[97m',
            'reset': '\033[0m',
            'bold': '\033[1m'
        }
    
    def find_serial_port(self):
        """自动查找ESP32串口"""
        patterns = [
            '/dev/cu.usbserial-*',
            '/dev/cu.SLAB_USBtoUART*',
            '/dev/cu.wchusbserial*',
            '/dev/ttyUSB*',
            '/dev/ttyACM*'
        ]
        
        ports = []
        for pattern in patterns:
            ports.extend(glob.glob(pattern))
        
        if ports:
            return sorted(ports)[-1]  # 返回最新的端口
        return None
    
    def generate_output_filename(self):
        """生成输出文件名：consoleout/YYYY-MM-DD_序号.log"""
        today = datetime.now().strftime("%Y-%m-%d")
        
        # 查找今天已有的文件
        pattern = os.path.join(self.output_dir, "{}_*.log".format(today))
        existing_files = glob.glob(pattern)
        
        if existing_files:
            # 提取序号并找到最大值
            numbers = []
            for file in existing_files:
                match = re.search(r"{}_(\d+)\.log".format(today), file)
                if match:
                    numbers.append(int(match.group(1)))
            next_num = max(numbers) + 1 if numbers else 1
        else:
            next_num = 1
        
        filename = "{}_{:02d}.log".format(today, next_num)
        return os.path.join(self.output_dir, filename)
    
    def colorize_line(self, line):
        """给日志行添加颜色"""
        line_lower = line.lower()
        
        # 错误相关 - 红色
        if any(keyword in line_lower for keyword in ['error', 'failed', 'fail', '失败', '错误']):
            return f"{self.colors['red']}{line}{self.colors['reset']}"
        
        # MQTT相关 - 蓝色
        elif 'mqtt' in line_lower:
            return f"{self.colors['blue']}{line}{self.colors['reset']}"
        
        # 成功相关 - 绿色
        elif any(keyword in line_lower for keyword in ['success', 'ok', 'connected', '成功', '连接']):
            return f"{self.colors['green']}{line}{self.colors['reset']}"
        
        # 警告相关 - 黄色
        elif any(keyword in line_lower for keyword in ['warning', 'warn', '警告']):
            return f"{self.colors['yellow']}{line}{self.colors['reset']}"
        
        # Air780EG相关 - 青色
        elif 'air780eg' in line_lower:
            return f"{self.colors['cyan']}{line}{self.colors['reset']}"
        
        # 调试信息 - 灰色
        elif '[debug]' in line_lower:
            return f"{self.colors['white']}{line}{self.colors['reset']}"
        
        return line
    
    def print_header(self):
        """打印监控开始信息"""
        print(f"{self.colors['bold']}{self.colors['cyan']}🚀 ESP32串口监控工具 - 增强版{self.colors['reset']}")
        print("=" * 60)
        print(f"🔌 串口: {self.colors['green']}{self.port}{self.colors['reset']}")
        print(f"📊 波特率: {self.colors['green']}{self.baudrate}{self.colors['reset']}")
        print(f"📁 输出文件: {self.colors['green']}{self.output_filename}{self.colors['reset']}")
        print(f"📂 输出目录: {self.colors['green']}{os.path.abspath(self.output_dir)}{self.colors['reset']}")
        print("=" * 60)
        print(f"✅ 开始监控... (Ctrl+C 停止)")
        print(f"💡 提示: 错误={self.colors['red']}红色{self.colors['reset']}, MQTT={self.colors['blue']}蓝色{self.colors['reset']}, 成功={self.colors['green']}绿色{self.colors['reset']}")
        print("=" * 60)
    
    def print_statistics(self):
        """打印统计信息"""
        if self.start_time:
            duration = time.time() - self.start_time
            print(f"\n{self.colors['bold']}{self.colors['cyan']}📊 监控统计{self.colors['reset']}")
            print("=" * 40)
            print(f"⏱️  监控时长: {duration:.1f} 秒")
            print(f"📝 总行数: {self.line_count}")
            print(f"❌ 错误数: {self.error_count}")
            print(f"📡 MQTT消息: {self.mqtt_count}")
            if duration > 0:
                print(f"📈 平均速率: {self.line_count/duration:.1f} 行/秒")
            print("=" * 40)
    
    def monitor_serial(self, duration=None, filter_pattern=None):
        """监控串口输出"""
        
        if not self.port:
            self.port = self.find_serial_port()
            if not self.port:
                print(f"{self.colors['red']}❌ 未找到串口设备{self.colors['reset']}")
                print("请检查ESP32是否已连接")
                return False
        
        self.output_filename = self.generate_output_filename()
        self.print_header()
        
        try:
            # 打开串口
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            time.sleep(2)  # 等待串口稳定
            
            # 打开输出文件
            with open(self.output_filename, 'w', encoding='utf-8') as f:
                self.output_file = f
                self.start_time = time.time()
                
                # 写入文件头
                timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                header = f"=== ESP32串口监控开始 {timestamp} ===\n"
                header += f"串口: {self.port}\n"
                header += f"波特率: {self.baudrate}\n"
                header += f"过滤器: {filter_pattern or '无'}\n"
                header += "=" * 50 + "\n"
                f.write(header)
                f.flush()
                
                end_time = time.time() + duration if duration else None
                
                while True:
                    try:
                        # 检查是否超时
                        if end_time and time.time() > end_time:
                            print(f"\n⏰ 监控时间到达 {duration} 秒，自动停止")
                            break
                        
                        # 读取串口数据
                        if self.ser.in_waiting > 0:
                            data = self.ser.readline()
                            try:
                                line = data.decode('utf-8', errors='ignore').rstrip()
                                if line:  # 只处理非空行
                                    self.line_count += 1
                                    
                                    # 应用过滤器
                                    if filter_pattern and filter_pattern.lower() not in line.lower():
                                        continue
                                    
                                    # 统计特殊行
                                    line_lower = line.lower()
                                    if any(keyword in line_lower for keyword in ['error', 'failed', 'fail', '失败', '错误']):
                                        self.error_count += 1
                                    if 'mqtt' in line_lower:
                                        self.mqtt_count += 1
                                    
                                    # 显示到控制台（带颜色）
                                    colored_line = self.colorize_line(line)
                                    print(f"[{self.line_count:04d}] {colored_line}")
                                    
                                    # 写入文件（不带颜色）
                                    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                                    f.write(f"[{timestamp}] {line}\n")
                                    f.flush()  # 立即刷新到文件
                                    
                            except UnicodeDecodeError:
                                # 忽略解码错误
                                pass
                        
                        time.sleep(0.01)  # 短暂延时
                        
                    except KeyboardInterrupt:
                        print(f"\n🛑 用户中断监控")
                        break
                    except Exception as e:
                        print(f"{self.colors['red']}❌ 读取错误: {e}{self.colors['reset']}")
                        time.sleep(1)
                
                # 写入文件尾
                end_timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                footer = f"\n=== 监控结束 {end_timestamp} ===\n"
                footer += f"总行数: {self.line_count}\n"
                footer += f"错误数: {self.error_count}\n"
                footer += f"MQTT消息: {self.mqtt_count}\n"
                f.write(footer)
                
        except serial.SerialException as e:
            print(f"{self.colors['red']}❌ 串口错误: {e}{self.colors['reset']}")
            print("请检查:")
            print("1. ESP32是否已连接")
            print("2. 串口是否被其他程序占用")
            print("3. 串口权限是否正确")
            return False
            
        except Exception as e:
            print(f"{self.colors['red']}❌ 未知错误: {e}{self.colors['reset']}")
            return False
            
        finally:
            if self.ser:
                try:
                    self.ser.close()
                    print(f"🔌 串口已关闭")
                except:
                    pass
            
            self.print_statistics()
            print(f"📁 日志已保存到: {self.colors['green']}{os.path.abspath(self.output_filename)}{self.colors['reset']}")
        
        return True

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='ESP32串口监控工具 - 增强版')
    parser.add_argument('-p', '--port', help='串口设备路径')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='波特率 (默认: 115200)')
    parser.add_argument('-d', '--duration', type=int, help='监控时长(秒)')
    parser.add_argument('-f', '--filter', help='过滤关键词')
    parser.add_argument('-o', '--output-dir', default='consoleout', help='输出目录 (默认: consoleout)')
    parser.add_argument('--list-ports', action='store_true', help='列出可用串口')
    
    args = parser.parse_args()
    
    # 列出可用串口
    if args.list_ports:
        monitor = SerialMonitor()
        ports = []
        patterns = ['/dev/cu.usbserial-*', '/dev/cu.SLAB_USBtoUART*', '/dev/cu.wchusbserial*']
        for pattern in patterns:
            ports.extend(glob.glob(pattern))
        
        if ports:
            print("🔌 可用串口:")
            for port in sorted(ports):
                print(f"  - {port}")
        else:
            print("❌ 未找到可用串口")
        return
    
    # 创建监控器
    monitor = SerialMonitor(
        port=args.port,
        baudrate=args.baudrate,
        output_dir=args.output_dir
    )
    
    # 开始监控
    monitor.monitor_serial(
        duration=args.duration,
        filter_pattern=args.filter
    )

if __name__ == "__main__":
    main()
