# ESP32-S3 MotoBox 开发指南

## 项目概述
本项目是基于ESP32-S3的摩托车数据采集与显示系统，专注于嵌入式系统开发和低功耗设计。

## 深度睡眠模式实现

### 休眠功能控制
从版本2.0.0开始，系统支持通过编译时配置来启用或禁用休眠功能：

#### 编译时配置
在`platformio.ini`文件中，通过`ENABLE_SLEEP`宏来控制休眠功能：
```ini
; 在[env]部分已定义默认启用休眠
-D ENABLE_SLEEP=1  ; 1=启用(默认) 0=禁用

; 在特定环境中禁用休眠，取消注释以下行:
; [env:allinone]
; build_flags = 
;   ...其他配置...
;   -D ENABLE_SLEEP=0  ; 禁用休眠功能
```

#### 休眠行为
- **启用休眠 (ENABLE_SLEEP=1)**：设备在1分钟无活动后，将进入倒计时阶段，随后进入深度睡眠模式
- **禁用休眠 (ENABLE_SLEEP=0)**：设备将始终保持在唤醒状态，不会进入睡眠模式，适合需要持续工作的场景

#### 运行时状态
- 设备启动时会在串口和显示屏上显示休眠功能的当前状态
- 休眠状态由编译时配置决定，运行时无法更改

### 睡眠模式唤醒源优先级
- **按钮唤醒**: 最高优先级，通过BTN_PIN配置
- **IMU运动检测唤醒**: 次优先级，通过IMU_INT1_PIN配置
- **定时器唤醒**: 最低优先级，作为兜底唤醒机制

### 唤醒处理流程
```cpp
// 唤醒源处理逻辑
void PowerManager::setupWakeupSources() {
  // 首先检查按钮是否为有效的RTC GPIO
  if (BTN_PIN >= 0 && BTN_PIN <= 21) {
    rtc_gpio_init((gpio_num_t)BTN_PIN);
    rtc_gpio_set_direction((gpio_num_t)BTN_PIN, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en((gpio_num_t)BTN_PIN);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_PIN, 0); 
    _wakeupSourceConfigured = true;
  }
  
  // 如果按钮无法配置，检查IMU中断引脚
  if (!_wakeupSourceConfigured && IMU_INT1_PIN >= 0 && IMU_INT1_PIN <= 21) {
    rtc_gpio_init((gpio_num_t)IMU_INT1_PIN);
    rtc_gpio_set_direction((gpio_num_t)IMU_INT1_PIN, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_en((gpio_num_t)IMU_INT1_PIN);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)IMU_INT1_PIN, 1);
    _wakeupSourceConfigured = true;
  }
  
  // 最后，始终配置定时器唤醒作为兜底机制
  esp_sleep_enable_timer_wakeup(WAKEUP_INTERVAL_US);
}
```

### 唤醒响应机制
- **按钮唤醒**: LED以BLINK_SINGLE模式闪烁，恢复系统
- **IMU运动唤醒**: 记录运动事件，可选择性唤醒或继续睡眠
- **定时器唤醒**: 检查系统状态，执行周期性任务

### IMU运动检测配置
- 通过QMI8658传感器的INT1引脚连接到ESP32-S3
- 配置传感器进行运动检测，超过阈值时触发中断
- 支持在深度睡眠状态下保持运动检测功能

### 显示系统状态
使用LED指示系统状态：
- **OFF**: 休眠状态
- **ON**: 正常工作
- **BLINK_SINGLE**: 唤醒或特殊事件
- **BLINK_DUAL**: 警告状态

### 电源管理优化
- 进入睡眠前关闭所有非必要外设
- 配置适当的RTC唤醒源
- 保持关键状态变量在RTC内存中

## 开发经验总结

### ESP32-S3 深度睡眠与唤醒

#### RTC GPIO 唤醒源限制
- ESP32-S3 仅支持 GPIO0-GPIO21 作为 RTC GPIO 唤醒源
- 深度睡眠唤醒只能使用这些特定的 GPIO 引脚
- 建议在硬件设计时，将按钮或唤醒信号连接到这些引脚

#### 唤醒源配置注意事项
```cpp
// 正确的RTC GPIO唤醒配置示例
if (BTN_PIN >= 0 && BTN_PIN <= 21) {
    rtc_gpio_init((gpio_num_t)BTN_PIN);
    rtc_gpio_set_direction((gpio_num_t)BTN_PIN, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en((gpio_num_t)BTN_PIN);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_PIN, 0); 
}
```

### 低功耗设计最佳实践

1. **外设管理**
   - 进入深度睡眠前关闭所有不必要的外设
   - 断开WiFi和MQTT连接
   - 使GPS模块进入待机模式
   - 关闭显示屏

2. **电源状态管理**
   - 使用状态机追踪设备电源状态
   - 提供倒计时机制，允许在进入深度睡眠前中断

3. **运动检测**
   - 使用IMU传感器检测设备运动
   - 动态调整运动检测阈值
   - 在低功耗模式下自动中断睡眠

### 常见陷阱与解决方案

#### GPIO 唤醒限制
- 问题：并非所有 GPIO 都支持深度睡眠唤醒
- 解决方案：
  1. 检查引脚是否为 RTC GPIO
  2. 必要时重新设计硬件连接
  3. 使用 `esp_sleep_enable_ext0_wakeup()` 配置唤醒源

#### 防止意外重启
- 添加按钮状态检查
- 配置上拉/下拉电阻
- 检查引脚电平，避免立即唤醒

### 性能优化建议

1. 使用 RTOS 任务管理
2. 合理配置任务优先级
3. 使用 `vTaskSuspend()` 和 `vTaskResume()` 管理任务

## 硬件兼容性

### ESP32-S3 特殊引脚
- GPIO26-GPIO32：连接到 SPI Flash 和 PSRAM，不建议使用
- GPIO0-GPIO21：RTC GPIO，适合深度睡眠唤醒
- 部分 GPIO 支持电容触摸

## 调试技巧

1. 使用 `Serial.printf()` 输出详细日志
2. 在关键位置添加状态输出
3. 使用 `millis()` 进行时间追踪

## 注意事项

- 深度睡眠会重置所有非 RTC 内存
- 使用 `RTC_DATA_ATTR` 保存关键变量
- 谨慎处理电源管理状态转换

## 许可证

本项目采用 GNU General Public License v3.0 (GPL-3.0) 开源许可证，并附加 Commons Clause 限制条款，禁止商业使用。

### 许可证主要条款

1. **使用与分发限制**
   - 您可以自由地使用、修改和分发本项目，但仅限于非商业用途
   - 禁止任何形式的商业使用，包括销售本软件或基于本软件的产品/服务

2. **开源要求**
   - 任何基于本项目的修改版本必须同样开源
   - 必须保留原始版权声明和许可证
   - 修改后的版本需要清晰标注所做的更改

3. **责任与担保**
   - 本项目按"原样"提供，不提供任何明确或隐含的担保
   - 作者不对使用本项目造成的任何直接或间接损失负责

4. **再分发限制**
   - 再分发时必须包含完整的源代码
   - 不得对原始许可证施加额外限制
   - 必须同时包含Commons Clause限制条款

### 商业使用限制

以下活动被明确禁止：

1. 销售本软件或基于本软件的产品
2. 提供基于本软件的付费服务
3. 将本软件集成到商业产品中
4. 利用本软件的功能提供商业服务

如需商业授权，请联系项目作者获取单独的商业许可。

### 完整许可证

详细许可证条款请参见项目根目录下的 `LICENSE` 文件。

有关GPL-3.0许可证详情，请访问：https://www.gnu.org/licenses/gpl-3.0.html
有关Commons Clause的详情，请访问：https://commonsclause.com/

### 贡献者协议

通过向本项目提交代码，您同意将您的贡献遵循GPL-3.0许可证和Commons Clause限制条款。

## 免责声明

本项目旨在提供有用的功能，但不保证完全没有缺陷。使用本项目所造成的任何损失，作者概不负责。

## 贡献

欢迎通过以下方式参与项目：
- 提交 Issues 报告 Bug 或建议新功能
- 发起 Pull Requests 改进代码
- 分享使用心得和二次开发经验

请遵守项目的代码风格和贡献准则。
