# MotoBox 项目文档

## 项目简介

MotoBox 是一个基于 ESP32-S3 的多功能车载信息系统，提供实时 GPS 定位、IMU 数据采集、蓝牙通信和 MQTT 数据传输等功能。

## 硬件配置

### 支持的硬件
- 开发板：ESP32-S3-DevKitC-1
- 显示屏：ILI9341 TFT 显示屏（172x320分辨率）
- GPS 模块：支持 NMEA 协议
- IMU 传感器：QMI8658
- 电池管理

### 引脚配置

#### 显示屏 (TFT)
- CS: 34
- MOSI: 35
- SCLK: 36
- DC: 7
- RST: 6

#### GPS
- RX: 12
- TX: 11
- 默认波特率: 9600

#### IMU
- SDA: 42
- SCL: 2

#### 电池
- 电压检测引脚: 7
- 电压范围: 2.9V - 3.25V

## 软件配置

### 运行模式

项目支持三种运行模式（在 `config.h` 中配置）：

1. `MODE_ALLINONE`：独立模式，自行采集数据并渲染
2. `MODE_SERVER`：主机模式，通过蓝牙发送数据，WIFI 发送 MQTT
3. `MODE_CLIENT`：从机模式，接收蓝牙数据并渲染

### MQTT 配置

- 服务器：mq-hub.daboluo.cc
- 端口：1883
- 主题：
  - 设备信息：`vehicle/v1/{device_id}/device/info`
  - GPS：`vehicle/v1/{device_id}/gps/position`
  - IMU：`vehicle/v1/{device_id}/imu/gyro`

## 开发环境

- 开发工具：PlatformIO
- 框架：Arduino
- 依赖库：
  - TinyGPSPlus
  - EspSoftwareSerial
  - NimBLE-Arduino
  - PubSubClient
  - ArduinoJson

## 编译与烧录

1. 安装 PlatformIO
2. 克隆项目
3. 执行 `pio run` 编译
4. 执行 `pio run -t upload` 烧录

## 使用说明

- 长按按键可重置 WiFi 配置
- 短按按键可切换 GPS 更新频率

## 许可证

### 非商业使用许可

本项目仅供个人学习和非商业用途。未经作者明确授权，禁止将本项目用于商业目的。

## 免责声明

本项目按"原样"提供，不保证稳定性和适用性。使用本项目所造成的任何损失，作者概不负责。

## 贡献

欢迎提交 Issues 和 Pull Requests！
