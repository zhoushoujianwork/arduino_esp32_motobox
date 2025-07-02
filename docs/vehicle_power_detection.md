# 车辆电门启动检测功能说明

## 功能概述

通过 `RTC_INT_PIN` 实现车辆电门启动供电检测和电源唤醒功能，当车辆启动时自动唤醒设备，车辆关闭时允许设备进入休眠状态。

**重要说明**：
- `RTC_INT_PIN`: 专门用于车辆电门启动检测
- `CHARGING_STATUS_PIN`: 专门用于充电状态检测（会因电池充满而变化，不适合作为车辆电门判断）

## 硬件连接

### 引脚配置
- **esp32-wifi环境**: `RTC_INT_PIN = GPIO35` (注释显示该环境未定义RTC_INT_PIN)
- **其他环境**: 根据 platformio.ini 中的配置

### 电路逻辑
- **车辆未启动**: 无外部供电，引脚为高电平（内部上拉）
- **车辆启动**: 外部5V供电，引脚被拉低到低电平

## 软件实现

### 1. 唤醒源配置

在 `PowerManager::configureWakeupSources()` 中：

```cpp
// 配置车辆电门启动检测唤醒 (RTC_INT_PIN)
#ifdef RTC_INT_PIN
    // 配置为RTC GPIO输入，启用内部上拉
    rtc_gpio_set_direction((gpio_num_t)RTC_INT_PIN, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en((gpio_num_t)RTC_INT_PIN);
    
    // 配置EXT1唤醒 - 当车辆启动时(从高电平变为低电平)唤醒
    uint64_t ext1_mask = 1ULL << RTC_INT_PIN;
    esp_sleep_enable_ext1_wakeup(ext1_mask, ESP_EXT1_WAKEUP_ALL_LOW);
#endif
```

### 2. 唤醒原因识别

在 `PowerManager::printWakeupReason()` 和 `PowerManager::checkWakeupCause()` 中：

```cpp
case ESP_SLEEP_WAKEUP_EXT1:
    // 检查是否是车辆电门启动唤醒
    if (digitalRead(RTC_INT_PIN) == LOW) {
        Serial.println("[系统] 🚗 检测到车辆电门启动！");
        Serial.println("[系统] 车辆开始供电，设备从休眠中唤醒");
    }
    break;
```

### 3. 运行时状态检测

在 `PowerManager::loop()` 中每秒检测一次车辆状态：

```cpp
// 每隔1秒检测一次车辆状态
if (now - lastVehicleCheck >= 1000) {
    lastVehicleCheck = now;
    handleVehicleStateChange();
}
```

### 4. 休眠阻止逻辑

在 `PowerManager::isDeviceIdle()` 中：

```cpp
// 检查车辆电门启动状态 - 如果车辆启动则不进入休眠
#ifdef RTC_INT_PIN
    bool vehicle_started = (digitalRead(RTC_INT_PIN) == LOW);
    if (vehicle_started) {
        // 车辆启动时，重置运动时间以防止进入休眠
        lastMotionTime = millis();
        return false;
    }
#endif
```

## 功能特性

### ✅ 支持的功能

1. **自动唤醒**: 车辆启动时从深度睡眠中唤醒设备
2. **休眠阻止**: 车辆启动期间阻止设备进入休眠
3. **状态监控**: 实时监控车辆电门状态变化
4. **倒计时取消**: 如果在休眠倒计时期间车辆启动，自动取消休眠
5. **多唤醒源**: 与IMU运动检测、定时器唤醒等其他唤醒源兼容
6. **充电状态独立**: 与充电状态检测分离，避免充满电时的误判

### 🔧 核心函数

- `isVehicleStarted()`: 检查车辆是否启动（基于RTC_INT_PIN）
- `handleVehicleStateChange()`: 处理车辆状态变化
- `checkGPIO39Stability()`: GPIO39稳定性检查（针对IMU引脚）

## 引脚分工

### RTC_INT_PIN (车辆电门检测)
- **用途**: 专门检测车辆电门启动状态
- **特点**: 只在车辆启动/关闭时变化，稳定可靠
- **逻辑**: 车辆启动时拉低，关闭时上拉为高

### CHARGING_STATUS_PIN (充电状态检测)
- **用途**: 专门检测充电状态
- **特点**: 会因充电开始、充电完成等状态变化
- **逻辑**: 充电时拉低，不充电时为高
- **注意**: 不适合用作车辆电门判断，因为电池充满时也会变化

## 使用场景

### 典型工作流程

1. **设备正常运行** → 检测到无运动且车辆未启动 → **进入休眠倒计时**
2. **深度睡眠状态** → 车辆启动(RTC_INT_PIN变为低电平) → **EXT1唤醒** → **设备唤醒**
3. **车辆运行期间** → 持续检测到车辆启动状态 → **阻止进入休眠**
4. **车辆关闭** → 检测到车辆停止状态 → **允许正常休眠流程**

### 日志输出示例

```
[电源管理] ✅ 车辆电门唤醒配置成功 (GPIO35, 低电平触发)
[系统] 🚗 检测到车辆电门启动！(GPIO35 = LOW)
[电源管理] 🚗 车辆电门启动检测到！
[电源管理] 设备将保持活跃状态
[电源管理] 🚗 车辆电门关闭检测到
[电源管理] 设备将根据运动状态决定是否休眠
```

## 配置说明

### platformio.ini 配置

```ini
# RTC 外部电源接入检测引脚，输入 5V 拉低，无输入时候高电平
-D RTC_INT_PIN=35

# 充电状态检测引脚，输入 5V 拉低，无输入时候高电平  
-D CHARGING_STATUS_PIN=2
```

### 兼容性

- ✅ 支持所有定义了 `RTC_INT_PIN` 的环境
- ✅ 与现有的IMU运动检测唤醒兼容
- ✅ 与定时器唤醒兼容
- ✅ 支持RTC GPIO引脚（GPIO0-GPIO21）
- ✅ 与充电状态检测功能独立，互不干扰

## 故障排除

### 常见问题

1. **设备无法唤醒**
   - 检查 `RTC_INT_PIN` 是否为有效的RTC GPIO
   - 确认硬件连接正确，车辆启动时引脚确实被拉低
   - 确认不是使用了 `CHARGING_STATUS_PIN` 来判断车辆状态

2. **误唤醒**
   - 检查引脚是否有外部干扰
   - 确认内部上拉电阻配置正确
   - 确认没有与充电状态检测混淆

3. **无法进入休眠**
   - 检查车辆状态检测逻辑
   - 确认 `isDeviceIdle()` 函数工作正常
   - 检查是否误用了充电状态作为车辆状态

### 调试方法

1. 监控串口输出查看车辆状态变化日志
2. 检查唤醒原因输出
3. 验证RTC_INT_PIN状态读取是否正确
4. 区分充电状态和车辆电门状态的变化

## 总结

通过 `RTC_INT_PIN` 实现的车辆电门启动检测功能提供了专门的车辆状态感知能力，与充电状态检测分离，确保设备在车辆使用期间保持活跃，在车辆停止后能够正常进入低功耗模式。这种设计避免了因电池充满等充电状态变化导致的误判，提供更可靠的车辆电门检测。
