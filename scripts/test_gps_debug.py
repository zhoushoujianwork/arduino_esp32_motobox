#!/usr/bin/env python3
"""
GPS调试测试脚本
用于验证GPS全链路调试功能是否正常工作
"""

import serial
import time
import re
import sys
import argparse

class GPSDebugTester:
    def __init__(self, port, baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.serial = None
        
        # 调试输出模式统计
        self.debug_stats = {
            'GPS': 0,
            'GNSS': 0, 
            'LBS': 0,
            'GPSManager': 0,
            'Air780EG': 0
        }
        
        # 调试输出模式检测正则
        self.debug_patterns = {
            'GPS': re.compile(r'\[GPS.*?\]'),
            'GNSS': re.compile(r'\[.*?GNSS.*?\]'),
            'LBS': re.compile(r'\[.*?LBS.*?\]'),
            'GPSManager': re.compile(r'\[GPSManager\]'),
            'Air780EG': re.compile(r'\[Air780EG.*?\]')
        }
    
    def connect(self):
        """连接串口"""
        try:
            self.serial = serial.Serial(self.port, self.baudrate, timeout=1)
            print(f"✅ 已连接到 {self.port} (波特率: {self.baudrate})")
            return True
        except Exception as e:
            print(f"❌ 连接失败: {e}")
            return False
    
    def disconnect(self):
        """断开串口连接"""
        if self.serial and self.serial.is_open:
            self.serial.close()
            print("🔌 串口已断开")
    
    def monitor_debug_output(self, duration=60):
        """监控调试输出"""
        if not self.serial or not self.serial.is_open:
            print("❌ 串口未连接")
            return False
        
        print(f"🔍 开始监控GPS调试输出 (持续 {duration} 秒)...")
        print("=" * 60)
        
        start_time = time.time()
        line_count = 0
        
        try:
            while time.time() - start_time < duration:
                if self.serial.in_waiting > 0:
                    try:
                        line = self.serial.readline().decode('utf-8', errors='ignore').strip()
                        if line:
                            line_count += 1
                            self.analyze_debug_line(line)
                            
                            # 实时显示调试输出
                            timestamp = time.strftime("%H:%M:%S")
                            print(f"[{timestamp}] {line}")
                            
                    except Exception as e:
                        print(f"⚠️ 读取数据错误: {e}")
                
                time.sleep(0.01)  # 避免CPU占用过高
                
        except KeyboardInterrupt:
            print("\n⏹️ 用户中断监控")
        
        print("=" * 60)
        print(f"📊 监控完成，共处理 {line_count} 行输出")
        self.print_debug_statistics()
        
        return True
    
    def analyze_debug_line(self, line):
        """分析调试输出行"""
        for debug_type, pattern in self.debug_patterns.items():
            if pattern.search(line):
                self.debug_stats[debug_type] += 1
                break
    
    def print_debug_statistics(self):
        """打印调试统计信息"""
        print("\n📈 GPS调试输出统计:")
        print("-" * 40)
        
        total_debug_lines = sum(self.debug_stats.values())
        
        for debug_type, count in self.debug_stats.items():
            if total_debug_lines > 0:
                percentage = (count / total_debug_lines) * 100
                print(f"{debug_type:12}: {count:4d} 行 ({percentage:5.1f}%)")
            else:
                print(f"{debug_type:12}: {count:4d} 行")
        
        print("-" * 40)
        print(f"{'总计':12}: {total_debug_lines:4d} 行")
        
        # 分析结果
        self.analyze_debug_health()
    
    def analyze_debug_health(self):
        """分析调试输出健康状况"""
        print("\n🏥 GPS调试健康检查:")
        print("-" * 40)
        
        issues = []
        
        # 检查是否有GPS相关输出
        if self.debug_stats['GPS'] == 0 and self.debug_stats['GNSS'] == 0:
            issues.append("❌ 未检测到GPS/GNSS调试输出")
        else:
            print("✅ GPS/GNSS调试输出正常")
        
        # 检查GPSManager输出
        if self.debug_stats['GPSManager'] == 0:
            issues.append("⚠️ 未检测到GPSManager调试输出")
        else:
            print("✅ GPSManager调试输出正常")
        
        # 检查Air780EG输出（如果启用）
        if self.debug_stats['Air780EG'] > 0:
            print("✅ Air780EG调试输出正常")
        
        # 检查LBS输出（如果启用）
        if self.debug_stats['LBS'] > 0:
            print("✅ LBS调试输出正常")
        
        if issues:
            print("\n⚠️ 发现的问题:")
            for issue in issues:
                print(f"  {issue}")
        else:
            print("\n🎉 GPS调试系统运行正常！")
    
    def send_test_commands(self):
        """发送测试命令"""
        if not self.serial or not self.serial.is_open:
            print("❌ 串口未连接")
            return False
        
        print("📤 发送GPS调试测试命令...")
        
        # 这里可以添加一些测试命令，比如通过MQTT发送GPS相关命令
        # 由于这是调试脚本，主要是监控输出，暂时不发送命令
        
        return True

def main():
    parser = argparse.ArgumentParser(description='GPS调试测试工具')
    parser.add_argument('--port', '-p', default='/dev/ttyUSB0', 
                       help='串口设备路径 (默认: /dev/ttyUSB0)')
    parser.add_argument('--baudrate', '-b', type=int, default=115200,
                       help='波特率 (默认: 115200)')
    parser.add_argument('--duration', '-d', type=int, default=60,
                       help='监控持续时间(秒) (默认: 60)')
    
    args = parser.parse_args()
    
    print("🛰️ GPS调试测试工具")
    print("=" * 50)
    
    tester = GPSDebugTester(args.port, args.baudrate)
    
    try:
        if not tester.connect():
            sys.exit(1)
        
        # 监控调试输出
        tester.monitor_debug_output(args.duration)
        
    except KeyboardInterrupt:
        print("\n⏹️ 程序被用户中断")
    except Exception as e:
        print(f"❌ 程序异常: {e}")
    finally:
        tester.disconnect()

if __name__ == "__main__":
    main()
