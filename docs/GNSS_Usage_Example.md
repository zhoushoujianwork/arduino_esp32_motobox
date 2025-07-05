# GNSS功能使用示例

## 概述

新的GPSManager提供了统一的定位管理功能，支持GPS和GNSS互斥配置，以及LBS辅助定位。

## 主要特性

### 1. 定位模式
- **GPS_ONLY**: 仅使用外部GPS模块
- **GNSS_ONLY**: 仅使用GSM模块内置GNSS
- **GNSS_WITH_LBS**: GNSS+LBS混合模式（推荐）
- **NONE**: 禁用所有定位功能

### 2. 支持的GNSS模块
- **Air780EG**: 支持GNSS和LBS
- **ML307**: 支持LBS（GNSS功能待实现）

## 基本使用

### 初始化
```cpp
#include "gps/GPSManager.h"

void setup() {
    // 初始化GPS管理器
    gpsManager.init();
    
    // 启用调试输出
    gpsManager.setDebug(true);
}
```

### 主循环
```cpp
void loop() {
    // 更新定位数据
    gpsManager.loop();
    
    // 检查定位状态
    if (gpsManager.isReady()) {
        Serial.println("定位可用");
        
        // 获取定位数据
        gps_data_t gpsData = gpsManager.getGPSData();
        Serial.printf("位置: %.6f, %.6f\n", gpsData.latitude, gpsData.longitude);
        Serial.printf("海拔: %.2f米\n", gpsData.altitude);
        Serial.printf("卫星数: %d\n", gpsData.satellites);
    }
    
    delay(1000);
}
```

## 高级功能

### 动态模式切换
```cpp
// 切换到GNSS+LBS混合模式
gpsManager.setLocationMode(LocationMode::GNSS_WITH_LBS);

// 切换到纯GNSS模式
gpsManager.setLocationMode(LocationMode::GNSS_ONLY);

// 禁用定位
gpsManager.setLocationMode(LocationMode::NONE);
```

### 单独控制GNSS和LBS
```cpp
// 启用/禁用GNSS
gpsManager.setGNSSEnabled(true);
gpsManager.setGNSSEnabled(false);

// 启用/禁用LBS
gpsManager.setLBSEnabled(true);
gpsManager.setLBSEnabled(false);

// 注意：GPS和GNSS是互斥的
gpsManager.setGPSEnabled(true);  // 会自动禁用GNSS
```

### 状态查询
```cpp
// 检查定位模式
LocationMode mode = gpsManager.getLocationMode();
Serial.printf("当前模式: %d\n", (int)mode);

// 检查模块类型
String moduleType = gpsManager.getGNSSModuleTypeString();
Serial.printf("GNSS模块: %s\n", moduleType.c_str());

// 检查各功能状态
bool gnssEnabled = gpsManager.isGNSSEnabled();
bool lbsEnabled = gpsManager.isLBSEnabled();
bool gpsEnabled = gpsManager.isGPSEnabled();

// 检查定位质量
bool gnssFixed = gpsManager.isGNSSFixed();
int satellites = gpsManager.getSatelliteCount();
float hdop = gpsManager.getHDOP();
```

### 数据获取
```cpp
// 获取GPS数据
gps_data_t gpsData = gpsManager.getGPSData();
bool gpsValid = gpsManager.isGPSDataValid();

// 获取LBS数据
lbs_data_t lbsData = gpsManager.getLBSData();
bool lbsValid = gpsManager.isLBSDataValid();

// 使用全局变量（兼容旧代码）
if (is_gps_data_valid(gps_data)) {
    Serial.printf("全局GPS数据: %.6f, %.6f\n", 
                  gps_data.latitude, gps_data.longitude);
}
```

## 配置选项

### platformio.ini配置
```ini
[env:esp32-air780eg]
build_flags = 
    -D ENABLE_AIR780EG    ; 启用Air780EG模块
    -D GSM_RX_PIN=25      ; GSM串口接收引脚
    -D GSM_TX_PIN=26      ; GSM串口发送引脚
    ; 其他配置...
```

### config.h配置
```cpp
// 自动配置（推荐）
#ifdef ENABLE_GSM
#ifndef ENABLE_GPS
#define ENABLE_GNSS   // 启用GNSS
#endif

#ifdef ENABLE_GNSS
#define ENABLE_LBS    // 启用LBS辅助定位
#endif
#endif
```

## 调试和故障排除

### 启用调试输出
```cpp
gpsManager.setDebug(true);
```

### 常见问题

1. **GNSS无法定位**
   - 检查天线连接
   - 确保在室外开阔环境
   - 等待足够的搜星时间（冷启动可能需要几分钟）

2. **LBS定位失败**
   - 检查网络连接状态
   - 确保SIM卡有效且有信号
   - 检查基站信息是否可用

3. **模式切换失败**
   - 检查硬件配置是否正确
   - 确保相关模块已正确初始化
   - 查看调试输出了解具体错误

### 调试命令（串口）
```
gnss.enable     - 启用GNSS
gnss.disable    - 禁用GNSS
lbs.enable      - 启用LBS
lbs.disable     - 禁用LBS
gps.status      - 查看GPS状态
```

## 性能优化

### 更新频率设置
```cpp
// 设置GNSS更新频率（仅Air780EG支持）
gpsManager.setGNSSUpdateRate(1);  // 1Hz
gpsManager.setGNSSUpdateRate(5);  // 5Hz（更高精度，更高功耗）
```

### 功耗优化
```cpp
// 在不需要定位时禁用功能
gpsManager.setLocationMode(LocationMode::NONE);

// 或仅在需要时启用LBS
gpsManager.setGNSSEnabled(false);
gpsManager.setLBSEnabled(true);
```

## 注意事项

1. **互斥性**: GPS和GNSS不能同时启用
2. **初始化顺序**: 确保网络模块在GPS管理器之前初始化
3. **内存管理**: 系统会自动管理字符串内存，无需手动释放
4. **线程安全**: 所有AT命令都有互斥锁保护
5. **错误处理**: 系统有自动重试和错误恢复机制

## 示例项目

完整的示例代码请参考：
- `src/main.cpp` - 主程序示例
- `src/device.cpp` - 设备管理示例
- `src/gps/GPSManager.cpp` - 实现细节
