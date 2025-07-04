# ESP32-S3 MotoBox 项目知识库



## 开发规则
dev分支开发，最终merge合并到 main分支，并附带标准 tag；
你可以编译上传后,自己通过脚本查看串口输出到 consoleout 然后查看进行排查
每次做功能开发的时候 不要再主分支上，在完成功能后我来手动合并，你要清理每次测试脚本，然后 commit每次变化到dev-xxx分支上

## 项目概述
ESP32-S3 MotoBox是一个基于ESP32-S3的摩托车数据采集与显示系统，专注于嵌入式系统开发和低功耗设计。该项目结合了多种传感器数据采集、无线通信和电源管理功能，为摩托车提供智能化解决方案。

## 核心功能

### 1. 低功耗管理系统
- **IMU唤醒功能**：利用QMI8658传感器的运动检测功能实现设备自动唤醒
- **深度睡眠机制**：静止一段时间后自动进入深度睡眠状态，大幅降低功耗
- **硬件唤醒实现**：通过IMU中断引脚连接到ESP32-S3的RTC GPIO实现唤醒
- **运动检测参数**：可配置的运动检测阈值和检测窗口

### 2. 网络连接功能
- **WiFi管理**：支持STA和AP两种模式，具备自动重连和配网功能
- **MQTT通信**：支持远程命令控制和数据上报
- **连接状态处理**：当WiFi断开时，MQTT不再进行请求，避免资源浪费
- **配网模式**：支持通过BLE指令或MQTT订阅指令进入配网模式

### 3. 电源管理策略
- **运动检测控制**：无运动达到阈值时开启休眠倒计时
- **AP模式特殊处理**：AP模式下有客户端连接时不进入休眠，保持服务
- **休眠安全机制**：休眠模式配置失败时会重试，防止设备无法唤醒
- **可配置休眠时间**：支持通过MQTT远程设置休眠时间参数

### 4. 状态指示系统
- **PWM灯光设计**：绿色5%常亮表示WiFi连接成功，具备联网能力
- **亮度优化**：优化PWM灯光亮度，防止出现闪光问题和解决常亮问题

### 5. 配置方式
- **BLE配置**：支持通过蓝牙低功耗进行设备配置
- **MQTT远程配置**：支持通过MQTT订阅指令进行远程配置
- **WiFi配置界面**：
  - 支持列表选择热点名称或用户手动输入
  - 用户输入密码
  - 配置完成后直接重启设备，无需校验
  - 重启后自动尝试使用已保存的WiFi配置连接网络

### 6. 音频反馈系统
- **NS4168功率放大芯片**：支持I2S数字音频输出，提供清晰的音频反馈
- **开机成功播报**：设备启动完成时播放上升音调序列
- **状态音频提示**：WiFi连接、GPS定位、低电量、睡眠模式等状态变化音频反馈
- **远程音频控制**：支持通过MQTT远程播放音频和测试音频系统
- **自定义音频播放**：支持指定频率和时长的自定义音频播放

## 版本历史

### v2.2.2 (2025-06-07)
- 优化振动监测阈值
- 设置PWM灯光亮度，防止闪光和常亮问题
- 增强电源管理功能
- 支持休眠时间设置与获取

### v2.2.1 (2025-06-07)
- 支持MQTT下发休眠时间配置

### v2.2.0 (2025-06-06)
- 支持MQTT配置

### v2.1.0 (2025-06-02)
- 支持低功耗模式
- 支持BLE配置设备进入组网模式

## 技术实现细节

### IMU唤醒功能实现
```cpp
// 运动检测相关方法
bool enableMotionDetection(int intPin, float threshold = MOTION_DETECTION_THRESHOLD_DEFAULT);
void disableMotionDetection();

// 深度睡眠前的IMU配置
bool configureForDeepSleep();

// 从深度睡眠唤醒后的IMU恢复
bool restoreFromDeepSleep();

// 检查IMU中断状态
bool isMotionDetected();
```

### PowerManager类集成
```cpp
// 配置IMU作为唤醒源
bool setupIMUWakeupSource(int intPin, float threshold = 0.05);

// 配置所有可用的唤醒源
void configureWakeupSources();
```

### 唤醒流程
1. 设备进入深度睡眠前，配置IMU为低功耗模式并启用运动检测
2. 将IMU中断引脚配置为RTC GPIO唤醒源
3. 当检测到运动时，IMU中断引脚触发，唤醒ESP32-S3
4. 系统启动后检查唤醒原因，并根据唤醒源执行相应操作

### 音频系统实现
```cpp
// 音频管理器初始化
bool AudioManager::begin(int ws_pin, int bclk_pin, int data_pin);

// 预定义音频事件播放
bool playBootSuccessSound();     // 开机成功音：上升音调序列
bool playWiFiConnectedSound();   // WiFi连接音：双音调
bool playGPSFixedSound();        // GPS定位音：三短音
bool playLowBatterySound();      // 低电量音：下降音调
bool playSleepModeSound();       // 睡眠音：渐弱音调

// 自定义音频播放
bool playCustomBeep(float frequency, int duration);
```

### I2S音频配置
```cpp
// NS4168引脚配置
#define IIS_S_WS_PIN    23  // Word Select
#define IIS_S_BCLK_PIN  22  // Bit Clock
#define IIS_S_DATA_PIN  21  // Data

// 音频参数
#define AUDIO_SAMPLE_RATE       16000  // 16kHz采样率
#define AUDIO_BITS_PER_SAMPLE   16     // 16位采样
#define AUDIO_CHANNELS          1      // 单声道
```

## 注意事项与最佳实践

1. **RTC GPIO限制**：ESP32-S3只有GPIO0-GPIO21可用作RTC GPIO，请确保IMU中断引脚连接到这些引脚之一
2. **中断引脚状态**：确保中断引脚在配置为唤醒源时不处于触发状态，否则设备将立即唤醒
3. **电源管理**：在深度睡眠前关闭所有不必要的外设，以最大化电池寿命
4. **去抖处理**：添加适当的去抖逻辑，避免误触发
5. **WiFi与休眠**：当设备处于WiFi AP配网模式且有客户端连接时，不应进入休眠状态
6. **休眠失败处理**：当休眠模式配置失败时，应重新尝试进入休眠模式，防止设备无法唤醒
7. **音频引脚冲突**：确保I2S音频引脚不与其他功能冲突，避免系统异常
8. **音频功耗管理**：音频播放会增加功耗，在低电量时应适当减少音频提示
9. **深度睡眠音频处理**：进入深度睡眠前会自动关闭I2S外设，唤醒后需重新初始化

## 许可证信息

本项目采用GNU General Public License v3.0 (GPL-3.0)开源许可证，并附加Commons Clause限制条款，禁止商业使用。未经授权，不得将本项目用于商业目的。
