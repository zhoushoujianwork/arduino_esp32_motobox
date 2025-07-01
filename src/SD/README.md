# SD卡管理器使用说明

## 概述
SDManager类提供了完整的SD卡管理功能，支持SDIO和SPI两种模式，包括固件升级、配置文件管理、日志记录和低功耗支持。

## 模式选择

### SPI模式（推荐用于引脚冲突情况）
```cpp
// 在config.h中定义
#define SD_MODE_SPI
#define SD_CS_PIN    5       // 片选引脚（可自定义）
#define SD_MOSI_PIN  23      // 主设备输出，从设备输入
#define SD_MISO_PIN  19      // 主设备输入，从设备输出  
#define SD_SCK_PIN   18      // 串行时钟
```

### SDIO模式（默认，性能更好）
```cpp
// 在config.h中不定义SD_MODE_SPI即为SDIO模式
// 使用ESP32默认SDIO引脚
```

## 功能特性

### 1. 基本操作
- SD卡初始化和状态检查
- 文件和目录操作
- 卡信息查询
- 自动模式检测和切换

### 2. 固件升级
- 自动检测固件更新文件
- 安全的OTA升级流程
- 固件验证和备份

### 3. 配置管理
- WiFi配置保存/加载
- 通用配置文件管理
- JSON格式支持

### 4. 日志系统
- 系统日志记录
- 日志文件管理
- 时间戳支持

### 5. 低功耗支持
- 深度睡眠前的SD卡准备
- 唤醒后的SD卡恢复
- 与PowerManager集成

## 使用方法

### 基本初始化
```cpp
#ifdef ENABLE_SDCARD
#include "SD/SDManager.h"

void setup() {
    // 启用调试输出
    sdManager.setDebug(true);
    
    // 初始化SD卡（自动检测SPI或SDIO模式）
    if (sdManager.begin()) {
        Serial.println("SD卡初始化成功");
        sdManager.printCardInfo();
    } else {
        Serial.println("SD卡初始化失败");
    }
}
#endif
```

### 高级SD卡管理功能

SDManager提供了完整的SD卡管理功能：

#### SD卡格式化
```cpp
// 格式化SD卡（会删除所有数据）
if (sdManager.formatSDCard()) {
    Serial.println("格式化成功");
} else {
    Serial.println("格式化失败");
}
```

#### SD卡健康检查
```cpp
// 检查SD卡健康状态
if (sdManager.checkSDCardHealth()) {
    Serial.println("SD卡状态良好");
} else {
    Serial.println("SD卡存在问题");
}
```

#### SD卡修复
```cpp
// 修复SD卡（重建目录结构）
sdManager.repairSDCard();
```

#### 串口命令处理
```cpp
// 在loop()中处理串口命令
void loop() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        if (!sdManager.handleSerialCommand(command)) {
            // 处理其他非SD命令
        }
    }
}
```

### 串口命令控制

SDManager内置串口命令处理器，支持以下命令：

- `sd_check` 或 `sdc` - 执行健康检查
- `sd_repair` 或 `sdr` - 修复SD卡
- `sd_format` 或 `sdf` - 格式化SD卡
- `sd_info` 或 `sdi` - 显示SD卡信息
- `sd_list` 或 `sdl` - 列出目录内容
- `sd_help` 或 `sdh` - 显示SD命令帮助

### 自动维护功能

系统会自动：
1. 检查关键目录是否存在（/config, /logs, /firmware, /data）
2. 执行读写测试验证SD卡功能
3. 监控存储空间使用情况
4. 记录维护操作日志
5. 创建操作标记文件（format_info.json, repair_info.json）

### 固件升级
```cpp
// 检查并执行固件升级
if (sdManager.hasFirmwareUpdate()) {
    Serial.println("发现固件更新");
    if (sdManager.performFirmwareUpdate()) {
        // 升级成功，设备将重启
    }
}
```

### 配置管理
```cpp
// 保存WiFi配置
sdManager.saveWiFiConfig("MyWiFi", "password123");

// 加载WiFi配置
String ssid, password;
if (sdManager.loadWiFiConfig(ssid, password)) {
    Serial.println("WiFi配置加载成功");
    Serial.println("SSID: " + ssid);
}

// 保存自定义配置
String config = "{\"setting1\":\"value1\",\"setting2\":123}";
sdManager.saveConfig(config, "/config/app.json");

// 加载自定义配置
String loadedConfig = sdManager.loadConfig("/config/app.json");
```

### 日志记录
```cpp
// 写入日志
sdManager.writeLog("系统启动完成");
sdManager.writeLog("传感器数据: 温度25°C", "/logs/sensor.log");

// 读取日志
String logs = sdManager.readLog("/logs/system.log", 10); // 读取最后10行
Serial.println(logs);

// 清空日志
sdManager.clearLog("/logs/old.log");
```

### 文件操作
```cpp
// 检查文件是否存在
if (sdManager.fileExists("/data/settings.txt")) {
    Serial.println("设置文件存在");
}

// 创建目录
sdManager.createDir("/data/backup");

// 列出目录内容
sdManager.listDir("/config");

// 删除文件
sdManager.deleteFile("/temp/old_data.txt");
```

## 文件结构建议

推荐的SD卡文件结构：
```
/
├── config/
│   ├── wifi.json          # WiFi配置
│   ├── app.json           # 应用配置
│   └── device.json        # 设备配置
├── logs/
│   ├── system.log         # 系统日志
│   ├── sensor.log         # 传感器日志
│   └── error.log          # 错误日志
├── firmware/
│   └── update.bin         # 固件更新文件
├── data/
│   └── backup/            # 数据备份
└── firmware.bin           # 主固件更新文件（根目录）
```

## 固件升级流程

1. 将新固件文件命名为 `firmware.bin` 放在SD卡根目录
2. 或者放在 `/firmware/update.bin`
3. 重启设备，系统会自动检测并升级
4. 升级完成后设备自动重启，固件文件会被删除

## 低功耗集成

SDManager已与PowerManager集成：
- 进入深度睡眠前自动断开SD卡连接
- 从睡眠唤醒后自动重新初始化SD卡
- 记录睡眠/唤醒事件日志

## 注意事项

1. **引脚配置**：确保SD卡引脚配置正确，不与其他功能冲突
2. **电源管理**：SD卡操作会增加功耗，在低功耗应用中需要合理使用
3. **文件系统**：建议使用FAT32格式的SD卡
4. **错误处理**：始终检查操作返回值，处理可能的错误情况
5. **调试模式**：开发时启用调试模式，生产环境建议关闭

## 错误排查

### SD卡初始化失败
- 检查引脚连接
- 确认SD卡格式（推荐FAT32）
- 检查SD卡是否损坏
- 验证电源供应是否稳定

### 固件升级失败
- 确认固件文件完整性
- 检查可用存储空间
- 验证固件文件大小合理（1KB-2MB）

### 配置文件问题
- 检查JSON格式是否正确
- 确认文件路径存在
- 验证文件权限
