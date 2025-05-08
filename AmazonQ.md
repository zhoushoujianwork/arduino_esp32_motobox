# ESP32-S3 MotoBox IMU唤醒功能实现指南

## 功能概述

本文档详细说明了如何使用IMU(惯性测量单元)的中断功能来实现ESP32-S3在静止一段时间后的自动唤醒功能。该功能允许设备在检测到运动时从深度睡眠状态中唤醒，从而实现低功耗与及时响应的平衡。

## 实现原理

1. **IMU运动检测**：利用QMI8658传感器的运动检测功能，当检测到加速度变化超过预设阈值时触发中断
2. **RTC GPIO唤醒**：将IMU中断引脚连接到ESP32-S3的RTC GPIO，配置为外部唤醒源
3. **低功耗模式**：在深度睡眠期间，仅保持IMU的加速度计在低功耗模式下工作，其他外设关闭

## 关键组件

### 1. IMU类增强功能

IMU类已增强以支持以下功能：

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

### 2. PowerManager类集成

PowerManager类已更新以利用IMU的运动检测功能：

```cpp
// 配置IMU作为唤醒源
bool setupIMUWakeupSource(int intPin, float threshold = 0.05);

// 配置所有可用的唤醒源
void configureWakeupSources();
```

## 配置参数

### IMU运动检测参数

```cpp
// 运动检测配置
#define MOTION_DETECTION_THRESHOLD_DEFAULT 0.05  // 默认运动检测阈值 (g)
#define MOTION_DETECTION_WINDOW_DEFAULT 5        // 默认运动检测窗口大小
#define MOTION_DETECTION_DEBOUNCE_MS 100         // 运动检测去抖时间 (ms)
```

### IMU中断引脚定义

```cpp
// 在device.h中定义
#ifndef IMU_INT1_PIN
#define IMU_INT1_PIN 4  // 默认使用GPIO4作为IMU中断引脚，请根据实际硬件修改
#endif
```

## 唤醒流程

1. 设备进入深度睡眠前，配置IMU为低功耗模式并启用运动检测
2. 将IMU中断引脚配置为RTC GPIO唤醒源
3. 当检测到运动时，IMU中断引脚触发，唤醒ESP32-S3
4. 系统启动后检查唤醒原因，并根据唤醒源执行相应操作

## 唤醒源处理

```cpp
void checkWakeupCause() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("[系统] 通过外部中断唤醒 (EXT0)");
      // 检查是否是IMU中断引脚唤醒
      if (IMU_INT1_PIN >= 0 && IMU_INT1_PIN <= 21) {
        if (digitalRead(IMU_INT1_PIN) == LOW) {
          Serial.println("[系统] 检测到IMU运动唤醒");
          // 这里可以添加特定的运动唤醒处理逻辑
        } else {
          Serial.println("[系统] 检测到按钮唤醒");
          // 这里可以添加特定的按钮唤醒处理逻辑
        }
      }
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("[系统] 通过定时器唤醒");
      break;
    default:
      Serial.printf("[系统] 唤醒原因: %d\n", wakeup_reason);
      break;
  }
}
```

## 注意事项

1. **RTC GPIO限制**：ESP32-S3只有GPIO0-GPIO21可用作RTC GPIO，请确保IMU中断引脚连接到这些引脚之一
2. **中断引脚状态**：确保中断引脚在配置为唤醒源时不处于触发状态，否则设备将立即唤醒
3. **电源管理**：在深度睡眠前关闭所有不必要的外设，以最大化电池寿命
4. **去抖处理**：添加适当的去抖逻辑，避免误触发

## 调试技巧

1. 使用串口监视器观察IMU中断状态和唤醒原因
2. 调整运动检测阈值以平衡灵敏度和功耗
3. 测试不同的运动模式，确保唤醒功能在各种场景下可靠工作

## 硬件连接

确保IMU中断引脚正确连接到ESP32-S3的RTC GPIO引脚，并根据实际硬件修改`IMU_INT1_PIN`定义。

## 功能测试

1. 编译并上传代码到设备
2. 观察设备进入深度睡眠状态
3. 轻轻移动或摇晃设备，观察是否唤醒
4. 检查串口输出，确认唤醒原因是IMU中断

## 进一步优化

1. 根据不同的使用场景动态调整运动检测阈值
2. 实现自适应功耗管理，根据电池电量调整唤醒灵敏度
3. 添加运动模式识别，区分不同类型的运动并执行相应操作
