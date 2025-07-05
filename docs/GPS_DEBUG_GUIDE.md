# GPS全链路调试功能使用指南

## 概述

本项目现已支持GPS全链路调试功能，通过独立的调试开关控制不同模块的调试输出，帮助开发者快速定位GPS相关问题。

## 调试开关配置

### 在 `config.h` 中的调试控制

```cpp
// GPS全链路调试输出控制
#ifndef GPS_DEBUG_ENABLED
#define GPS_DEBUG_ENABLED             false
#endif

// GNSS调试输出控制（Air780EG GNSS功能）
#ifndef GNSS_DEBUG_ENABLED
#define GNSS_DEBUG_ENABLED            false
#endif

// LBS调试输出控制（基站定位功能）
#ifndef LBS_DEBUG_ENABLED
#define LBS_DEBUG_ENABLED             false
#endif
```

### 在 `platformio.ini` 中启用调试

```ini
[env:your_environment]
build_flags = 
    -DGPS_DEBUG_ENABLED=true
    -DGNSS_DEBUG_ENABLED=true
    -DLBS_DEBUG_ENABLED=true
```

## 调试宏使用

### GPS调试宏
```cpp
GPS_DEBUG_PRINT(x)            // 打印不换行
GPS_DEBUG_PRINTLN(x)          // 打印并换行
GPS_DEBUG_PRINTF(fmt, ...)    // 格式化打印
```

### GNSS调试宏
```cpp
GNSS_DEBUG_PRINT(x)           // 打印不换行
GNSS_DEBUG_PRINTLN(x)         // 打印并换行
GNSS_DEBUG_PRINTF(fmt, ...)   // 格式化打印
```

### LBS调试宏
```cpp
LBS_DEBUG_PRINT(x)            // 打印不换行
LBS_DEBUG_PRINTLN(x)          // 打印并换行
LBS_DEBUG_PRINTF(fmt, ...)    // 格式化打印
```

## 支持的模块

### 1. GPSManager
- 统一管理GPS、GNSS和LBS定位
- 模式切换调试信息
- 数据更新状态跟踪

**调试输出示例：**
```
[GPSManager] 开始初始化定位管理器
[GPSManager] 检测到Air780EG GNSS模块
[GPSManager] 配置为GNSS+LBS混合模式
[GPSManager] GNSS数据更新: 31.230416, 121.473701, 高度: 12.3m, 卫星: 8
[GPSManager] LBS数据更新: 31.230500, 121.473800
```

### 2. Air780EG GNSS适配器
- GNSS功能启用/禁用
- 定位数据解析
- 卫星信号状态

**调试输出示例：**
```
[Air780EG-GNSS] 开始初始化GNSS适配器
[Air780EG-GNSS] GNSS适配器初始化成功
[Air780EG-GNSS] 位置更新: 31.230416, 121.473701, 高度: 12.3m, 卫星: 8
```

### 3. Air780EG Modem
- AT指令交互
- GNSS电源管理
- LBS基站定位

**调试输出示例：**
```
[Air780EG] 启用GNSS
[Air780EG] GNSS电源开启成功
[Air780EG] GNSS数据: Lat=31.230416, Lon=121.473701, Sats=8
[Air780EG] 启用LBS定位
```

## 使用场景

### 场景1：GPS定位问题排查
```ini
# 启用GPS相关调试
build_flags = 
    -DGPS_DEBUG_ENABLED=true
    -DGNSS_DEBUG_ENABLED=true
```

### 场景2：Air780EG GNSS问题排查
```ini
# 启用GNSS和LBS调试
build_flags = 
    -DGNSS_DEBUG_ENABLED=true
    -DLBS_DEBUG_ENABLED=true
```

### 场景3：完整GPS链路调试
```ini
# 启用所有GPS相关调试
build_flags = 
    -DGPS_DEBUG_ENABLED=true
    -DGNSS_DEBUG_ENABLED=true
    -DLBS_DEBUG_ENABLED=true
    -DSYSTEM_DEBUG_ENABLED=true
```

## 调试工具

### GPS调试测试脚本
项目提供了专门的GPS调试测试工具：

```bash
# 基本使用
python3 scripts/test_gps_debug.py --port /dev/ttyUSB0

# 指定监控时间
python3 scripts/test_gps_debug.py --port /dev/ttyUSB0 --duration 120

# 指定波特率
python3 scripts/test_gps_debug.py --port /dev/ttyUSB0 --baudrate 115200
```

**功能特性：**
- 实时监控GPS调试输出
- 统计不同模块的调试信息数量
- 分析GPS调试系统健康状况
- 自动检测调试输出异常

## 常见问题排查

### 1. 无GPS调试输出
**可能原因：**
- 调试开关未启用
- GPS模块未初始化
- 串口连接问题

**解决方案：**
```cpp
// 检查config.h中的调试开关
#define GPS_DEBUG_ENABLED true

// 检查GPSManager初始化
gpsManager.init();
gpsManager.setDebug(true);
```

### 2. GNSS数据无效
**可能原因：**
- GNSS模块未启用
- 信号接收不良
- AT指令执行失败

**解决方案：**
```cpp
// 启用GNSS调试查看详细信息
#define GNSS_DEBUG_ENABLED true

// 检查GNSS启用状态
if (!air780eg_modem.isGNSSEnabled()) {
    air780eg_modem.enableGNSS(true);
}
```

### 3. LBS定位失败
**可能原因：**
- 网络连接问题
- 基站信息不足
- LBS服务未启用

**解决方案：**
```cpp
// 启用LBS调试
#define LBS_DEBUG_ENABLED true

// 检查网络状态
if (air780eg_modem.isNetworkReady()) {
    air780eg_modem.enableLBS(true);
}
```

## 性能考虑

### 调试输出对性能的影响
- **串口输出延迟**：每条调试信息约增加1-5ms延迟
- **内存占用**：字符串格式化会增加堆内存使用
- **CPU占用**：调试输出会占用额外的CPU时间

### 生产环境建议
```ini
# 生产环境关闭所有调试
build_flags = 
    -DGPS_DEBUG_ENABLED=false
    -DGNSS_DEBUG_ENABLED=false
    -DLBS_DEBUG_ENABLED=false
```

## 最佳实践

### 1. 分层调试
- 先启用系统级调试（SYSTEM_DEBUG）
- 再启用GPS管理器调试（GPS_DEBUG）
- 最后启用具体模块调试（GNSS_DEBUG/LBS_DEBUG）

### 2. 问题定位流程
1. 检查GPSManager初始化状态
2. 确认定位模式配置正确
3. 验证硬件模块通信正常
4. 分析定位数据有效性

### 3. 调试信息过滤
使用串口监控工具时，可以通过关键字过滤：
```bash
# 只显示GPS相关信息
python3 monitor_serial.py | grep -E "\[GPS|\[GNSS|\[LBS"

# 只显示错误信息
python3 monitor_serial.py | grep -E "错误|失败|异常"
```

## 更新日志

### v2.3.0 (2025-07-05)
- ✅ 新增GPS全链路调试功能
- ✅ 支持独立的GPS、GNSS、LBS调试开关
- ✅ Air780EG GNSS适配器集成到GPSManager
- ✅ 提供GPS调试测试工具
- ✅ 完善调试输出格式和内容

### 未来计划
- [ ] 支持调试输出到SD卡
- [ ] 增加GPS性能分析工具
- [ ] 支持远程调试控制
- [ ] 添加GPS轨迹记录功能
