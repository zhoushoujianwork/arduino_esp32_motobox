# GPS数据获取优化说明

## 优化目标
解决频繁调用 `AT+CGNSINF` 获取GPS信息的性能问题，通过在loop中每秒执行一次，成功后更新数据结构到GPSManager的数据结构体上，避免每次都单独进行AT调用。

## 优化方案

### 1. 新增文件
- `GPSDataCache.h` - GPS数据缓存管理器头文件
- `GPSDataCache.cpp` - GPS数据缓存管理器实现
- `GPSOptimizedExample.cpp` - 使用示例

### 2. 修改文件
- `GPSManager.h` - 添加GPS数据缓存支持
- `GPSManager.cpp` - 优化数据更新逻辑

### 3. 核心优化点

#### 3.1 统一数据获取
```cpp
// 原来：多处调用AT命令
String response1 = modem.sendATWithResponse("AT+CGNSINF", 3000);
String response2 = modem.sendATWithResponse("AT+CGNSINF", 3000);

// 现在：统一缓存管理
gpsDataCache.loop(); // 每秒调用一次AT+CGNSINF
gps_data_t data = gpsDataCache.getGPSData(); // 从缓存读取
```

#### 3.2 频率控制
```cpp
// GPSDataCache每秒更新一次
void GPSDataCache::loop() {
    if (millis() - _lastUpdateTime < 1000) return;
    
    // 调用AT+CGNSINF并解析数据
    updateGPSDataFromModem();
    _lastUpdateTime = millis();
}
```

#### 3.3 数据访问优化
```cpp
// GPSManager直接从缓存获取数据
void GPSManager::handleGNSSUpdate() {
    if (gpsDataCache.isDataValid()) {
        _gpsData = gpsDataCache.getGPSData(); // 无AT调用
    }
}
```

## 使用方法

### 1. 在主程序setup()中
```cpp
void setup() {
    // 初始化GPS管理器（自动初始化缓存）
    gpsManager.init();
    gpsManager.setLocationMode(LocationMode::GNSS_ONLY);
    gpsManager.setGNSSEnabled(true);
}
```

### 2. 在主程序loop()中
```cpp
void loop() {
    // 只需要调用一次，内部自动管理AT调用频率
    gpsManager.loop();
    
    // 随时获取数据，不触发AT调用
    if (gpsManager.isGPSDataValid()) {
        gps_data_t data = gpsManager.getGPSData();
        // 使用GPS数据...
    }
}
```

## 性能提升

### 1. 减少AT调用次数
- **原来**: 每次获取数据都调用AT+CGNSINF
- **现在**: 每秒统一调用一次，数据缓存供多次使用

### 2. 提高响应速度
- **原来**: 每次获取需要等待3秒AT响应
- **现在**: 从内存缓存立即获取数据

### 3. 降低资源占用
- 减少串口通信开销
- 降低CPU占用率
- 提高系统整体性能

## 兼容性
- 保持原有GPSManager接口不变
- 向后兼容现有代码
- 可选择性启用优化功能

## 编译配置
确保在config.h中启用相关宏定义：
```cpp
#define USE_AIR780EG_GSM  // 启用Air780EG支持
#define GPS_DEBUG_ENABLED true  // 启用调试输出
```

## 注意事项
1. 需要确保Air780EG模块已正确初始化
2. GPS数据缓存会在网络未就绪时自动重置
3. 建议在使用前检查数据新鲜度：`gpsDataCache.isDataFresh()`
