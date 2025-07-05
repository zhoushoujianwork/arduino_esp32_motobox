#!/bin/bash

# GPS全链路调试功能提交脚本
# 用于提交GPS调试功能相关的所有更改

echo "🛰️ GPS全链路调试功能提交脚本"
echo "=================================="

# 检查是否在正确的目录
if [ ! -f "platformio.ini" ]; then
    echo "❌ 错误：请在项目根目录运行此脚本"
    exit 1
fi

# 显示当前分支
current_branch=$(git branch --show-current)
echo "📍 当前分支: $current_branch"

# 检查是否有未提交的更改
if ! git diff --quiet; then
    echo "📝 检测到未提交的更改"
    
    # 显示更改的文件
    echo "📋 更改的文件列表:"
    git status --porcelain
    
    echo ""
    echo "🔍 主要更改内容:"
    echo "  • config.h - 增加GPS全链路调试控制宏"
    echo "  • GPSManager.h/cpp - 集成Air780EG GNSS适配器"
    echo "  • Air780EGGNSSAdapter.h/cpp - 新增GNSS适配器"
    echo "  • Air780EGModem.cpp - 更新调试输出使用新宏"
    echo "  • test_gps_debug.py - GPS调试测试工具"
    echo "  • GPS_DEBUG_GUIDE.md - GPS调试使用指南"
    echo "  • AmazonQ.md - 更新版本历史和技术文档"
    
    echo ""
    read -p "🤔 是否要提交这些更改? (y/N): " confirm
    
    if [[ $confirm =~ ^[Yy]$ ]]; then
        echo "📦 准备提交更改..."
        
        # 添加所有更改的文件
        git add src/config.h
        git add src/gps/GPSManager.h
        git add src/gps/GPSManager.cpp
        git add src/gps/Air780EGGNSSAdapter.h
        git add src/gps/Air780EGGNSSAdapter.cpp
        git add src/net/Air780EGModem.cpp
        git add scripts/test_gps_debug.py
        git add docs/GPS_DEBUG_GUIDE.md
        git add AmazonQ.md
        
        # 提交更改
        commit_message="feat: 新增GPS全链路调试功能

- 在config.h中增加GPS、GNSS、LBS独立调试控制宏
- GPSManager完全接管Air780EG的GNSS功能
- 创建Air780EGGNSSAdapter适配器实现统一GPS接口
- 通过AT指令获取Air780EG GNSS定位数据
- 更新所有GPS相关模块使用新的调试宏
- 提供GPS调试测试工具和完整使用文档
- 优化调试输出格式，提供更清晰的问题定位信息

版本: v2.3.1"
        
        git commit -m "$commit_message"
        
        if [ $? -eq 0 ]; then
            echo "✅ 提交成功！"
            echo ""
            echo "📋 提交信息:"
            git log --oneline -1
            echo ""
            echo "🏷️ 建议创建标签:"
            echo "   git tag -a v2.3.1 -m 'GPS全链路调试功能'"
            echo "   git push origin v2.3.1"
            echo ""
            echo "🚀 推送到远程仓库:"
            echo "   git push origin $current_branch"
        else
            echo "❌ 提交失败！"
            exit 1
        fi
    else
        echo "⏹️ 取消提交"
    fi
else
    echo "✅ 没有检测到未提交的更改"
fi

echo ""
echo "🎉 GPS全链路调试功能开发完成！"
echo ""
echo "📖 使用说明:"
echo "  • 查看 docs/GPS_DEBUG_GUIDE.md 了解详细使用方法"
echo "  • 使用 scripts/test_gps_debug.py 测试调试功能"
echo "  • 在 platformio.ini 中配置调试开关"
echo ""
echo "🔧 调试开关配置示例:"
echo "  build_flags = "
echo "    -DGPS_DEBUG_ENABLED=true"
echo "    -DGNSS_DEBUG_ENABLED=true"
echo "    -DLBS_DEBUG_ENABLED=true"
