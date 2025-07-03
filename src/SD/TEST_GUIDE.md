# SD卡功能测试指南

## 简化版SD卡功能概述

当前版本的SD卡管理器已经简化，只保留核心功能：
- 设备信息存储（JSON格式）
- GPS数据记录（GeoJSON格式）
- 基本的空间信息查询

## 硬件连接

### SPI模式引脚配置
- CS: GPIO16
- MOSI: GPIO17  
- MISO: GPIO18
- SCK: GPIO5

确保SD卡模块正确连接到这些引脚。

## 串口命令测试

连接串口监视器（波特率115200），可以使用以下命令测试SD卡功能：

### 基本命令
```
help                    # 显示所有可用命令
info                    # 显示设备信息（包括SD卡状态）
```

### SD卡专用命令
```
sd.info                 # 显示SD卡详细信息
sd.test                 # 测试GPS数据记录功能
```

## 预期输出示例

### 设备信息输出
```
=== 设备信息 ===
设备ID: AA:BB:CC:DD:EE:FF
固件版本: 2.3.0
WiFi状态: 已连接
GPS状态: 就绪
SD卡状态: 就绪
SD卡容量: 32768 MB
SD卡剩余: 32000 MB
```

### SD卡信息输出
```
=== SD卡信息 ===
设备ID: AA:BB:CC:DD:EE:FF
总容量: 32768 MB
剩余空间: 32000 MB
初始化状态: 已初始化
```

### GPS测试输出
```
测试GPS数据记录...
📍 GPS数据已记录: 39.904200,116.407400
测试结果: 成功
```

## 文件结构

SD卡初始化成功后，会自动创建以下目录结构：

```
/
├── config/
│   └── device_info.json    # 设备信息文件
└── data/
    └── gps/
        └── gps_XXXX.geojson # GPS数据文件（按日期命名）
```

## 设备信息文件格式

`/config/device_info.json` 文件内容示例：
```json
{
  "device_id": "AA:BB:CC:DD:EE:FF",
  "firmware_version": "2.3.0",
  "created_at": "123456789",
  "sd_total_mb": 32768,
  "sd_free_mb": 32000
}
```

## GPS数据文件格式

GPS数据以GeoJSON格式存储，文件示例：
```json
{
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "geometry": {
        "type": "Point",
        "coordinates": [116.407400, 39.904200, 50.00]
      },
      "properties": {
        "timestamp": "123456789",
        "speed": 25.50,
        "satellites": 8
      }
    }
  ]
}
```

## 故障排除

### SD卡初始化失败
1. 检查硬件连接是否正确
2. 确认SD卡格式为FAT32
3. 检查SD卡是否损坏
4. 验证引脚配置是否与实际硬件匹配

### GPS数据记录失败
1. 确认SD卡已正确初始化
2. 检查SD卡剩余空间
3. 验证GPS数据是否有效

### 串口命令无响应
1. 确认串口波特率设置为115200
2. 检查串口连接是否正常
3. 确认固件已正确烧录

## 注意事项

1. 当前版本为简化版，不支持复杂的数据管理功能
2. GPS数据文件按日期自动分割（基于系统运行时间）
3. 设备ID基于ESP32的MAC地址生成
4. 时间戳使用系统运行时间（毫秒），实际项目中应使用RTC或NTP时间
