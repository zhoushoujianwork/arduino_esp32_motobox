#!/usr/bin/env python3
"""
MQTT调试模式切换脚本
用于快速启用/禁用不同类型的调试输出
"""

import os
import sys
import re

def read_config_file(config_path):
    """读取配置文件"""
    try:
        with open(config_path, 'r', encoding='utf-8') as f:
            return f.read()
    except FileNotFoundError:
        print(f"错误: 找不到配置文件 {config_path}")
        return None

def write_config_file(config_path, content):
    """写入配置文件"""
    try:
        with open(config_path, 'w', encoding='utf-8') as f:
            f.write(content)
        return True
    except Exception as e:
        print(f"错误: 写入配置文件失败 {e}")
        return False

def toggle_debug_setting(content, debug_type, enable):
    """切换调试设置"""
    pattern = rf'(#define\s+{debug_type}_DEBUG_ENABLED\s+)(true|false)'
    replacement = rf'\g<1>{"true" if enable else "false"}'
    
    new_content = re.sub(pattern, replacement, content)
    
    if new_content == content:
        print(f"警告: 未找到 {debug_type}_DEBUG_ENABLED 设置")
        return content
    
    return new_content

def get_current_settings(content):
    """获取当前调试设置"""
    settings = {}
    
    patterns = {
        'MQTT': r'#define\s+MQTT_DEBUG_ENABLED\s+(true|false)',
        'NETWORK': r'#define\s+NETWORK_DEBUG_ENABLED\s+(true|false)',
        'SYSTEM': r'#define\s+SYSTEM_DEBUG_ENABLED\s+(true|false)'
    }
    
    for debug_type, pattern in patterns.items():
        match = re.search(pattern, content)
        if match:
            settings[debug_type] = match.group(1) == 'true'
        else:
            settings[debug_type] = None
    
    return settings

def main():
    # 获取项目根目录
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    config_path = os.path.join(project_root, 'src', 'config.h')
    
    if len(sys.argv) < 2:
        print("MQTT调试模式切换脚本")
        print("用法:")
        print("  python toggle_debug.py status                    - 显示当前调试状态")
        print("  python toggle_debug.py mqtt on|off              - 切换MQTT调试")
        print("  python toggle_debug.py network on|off           - 切换网络调试")
        print("  python toggle_debug.py system on|off            - 切换系统调试")
        print("  python toggle_debug.py all on|off               - 切换所有调试")
        print("  python toggle_debug.py quiet                    - 关闭所有调试（静默模式）")
        print("  python toggle_debug.py verbose                  - 开启所有调试（详细模式）")
        return
    
    # 读取配置文件
    content = read_config_file(config_path)
    if content is None:
        return
    
    command = sys.argv[1].lower()
    
    if command == 'status':
        # 显示当前状态
        settings = get_current_settings(content)
        print("当前调试设置:")
        for debug_type, enabled in settings.items():
            if enabled is not None:
                status = "启用" if enabled else "禁用"
                print(f"  {debug_type:8} 调试: {status}")
            else:
                print(f"  {debug_type:8} 调试: 未找到设置")
        return
    
    if command == 'quiet':
        # 静默模式 - 关闭所有调试
        content = toggle_debug_setting(content, 'MQTT', False)
        content = toggle_debug_setting(content, 'NETWORK', False)
        content = toggle_debug_setting(content, 'SYSTEM', False)
        
        if write_config_file(config_path, content):
            print("✅ 已切换到静默模式（所有调试已关闭）")
        return
    
    if command == 'verbose':
        # 详细模式 - 开启所有调试
        content = toggle_debug_setting(content, 'MQTT', True)
        content = toggle_debug_setting(content, 'NETWORK', True)
        content = toggle_debug_setting(content, 'SYSTEM', True)
        
        if write_config_file(config_path, content):
            print("✅ 已切换到详细模式（所有调试已开启）")
        return
    
    if len(sys.argv) < 3:
        print("错误: 缺少参数")
        return
    
    debug_type = command.upper()
    action = sys.argv[2].lower()
    
    if action not in ['on', 'off']:
        print("错误: 操作必须是 'on' 或 'off'")
        return
    
    enable = action == 'on'
    
    if debug_type == 'ALL':
        # 切换所有调试
        content = toggle_debug_setting(content, 'MQTT', enable)
        content = toggle_debug_setting(content, 'NETWORK', enable)
        content = toggle_debug_setting(content, 'SYSTEM', enable)
        
        if write_config_file(config_path, content):
            status = "启用" if enable else "禁用"
            print(f"✅ 已{status}所有调试输出")
    elif debug_type in ['MQTT', 'NETWORK', 'SYSTEM']:
        # 切换特定调试
        content = toggle_debug_setting(content, debug_type, enable)
        
        if write_config_file(config_path, content):
            status = "启用" if enable else "禁用"
            print(f"✅ 已{status}{debug_type}调试输出")
    else:
        print(f"错误: 未知的调试类型 '{debug_type}'")
        print("支持的类型: mqtt, network, system, all")

if __name__ == '__main__':
    main()
