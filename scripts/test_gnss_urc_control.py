#!/usr/bin/env python3
"""
GNSS自动上报控制测试脚本
用于测试Air780EG模块的GNSS URC控制功能
"""

import serial
import time
import sys

def send_at_command(ser, command, timeout=3):
    """发送AT命令并等待响应"""
    print(f"发送: {command}")
    ser.write((command + '\r\n').encode())
    
    start_time = time.time()
    response = ""
    
    while time.time() - start_time < timeout:
        if ser.in_waiting:
            data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            response += data
            print(f"接收: {data.strip()}")
            
            if "OK" in response or "ERROR" in response:
                break
        time.sleep(0.1)
    
    return response

def test_gnss_urc_control():
    """测试GNSS URC控制"""
    # 配置串口
    port = "/dev/cu.usbserial-10"  # 根据实际情况修改
    baudrate = 115200
    
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"已连接到 {port}")
        
        # 等待模块启动
        time.sleep(2)
        
        # 测试基本AT命令
        print("\n=== 测试基本AT命令 ===")
        send_at_command(ser, "AT")
        
        # 检查GNSS电源状态
        print("\n=== 检查GNSS电源状态 ===")
        send_at_command(ser, "AT+CGNSPWR?")
        
        # 检查当前GNSS URC设置
        print("\n=== 检查当前GNSS URC设置 ===")
        send_at_command(ser, "AT+CGNSURC?")
        
        # 关闭GNSS自动上报
        print("\n=== 关闭GNSS自动上报 ===")
        response = send_at_command(ser, "AT+CGNSURC=0")
        if "OK" in response:
            print("✅ GNSS自动上报已关闭")
        else:
            print("❌ GNSS自动上报关闭失败")
        
        # 验证设置
        print("\n=== 验证GNSS URC设置 ===")
        send_at_command(ser, "AT+CGNSURC?")
        
        # 等待一段时间，观察是否还有URC消息
        print("\n=== 等待10秒，观察URC消息 ===")
        start_time = time.time()
        urc_count = 0
        
        while time.time() - start_time < 10:
            if ser.in_waiting:
                data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                if "+UGNSINF:" in data:
                    urc_count += 1
                    print(f"⚠️  仍收到GNSS URC: {data.strip()}")
                elif data.strip():
                    print(f"其他数据: {data.strip()}")
            time.sleep(0.1)
        
        if urc_count == 0:
            print("✅ 10秒内未收到GNSS URC消息，关闭成功")
        else:
            print(f"❌ 仍收到 {urc_count} 条GNSS URC消息")
        
        # 可选：重新启用GNSS自动上报
        print("\n=== 重新启用GNSS自动上报（可选） ===")
        choice = input("是否重新启用GNSS自动上报？(y/n): ")
        if choice.lower() == 'y':
            response = send_at_command(ser, "AT+CGNSURC=1")
            if "OK" in response:
                print("✅ GNSS自动上报已重新启用")
            else:
                print("❌ GNSS自动上报启用失败")
        
        ser.close()
        print("\n测试完成")
        
    except serial.SerialException as e:
        print(f"串口错误: {e}")
        print("请检查串口设备是否正确连接")
    except KeyboardInterrupt:
        print("\n测试被用户中断")
    except Exception as e:
        print(f"发生错误: {e}")

if __name__ == "__main__":
    print("Air780EG GNSS自动上报控制测试")
    print("=" * 40)
    test_gnss_urc_control()
