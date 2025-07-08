# Air780EG库集成指南

本指南将帮助你将现有的Air780EG相关代码迁移到新的Air780EG库。

## 1. 库结构概览

新的Air780EG库采用分层设计：

```
Air780EG (主控制类)
├── Air780EGCore (AT指令核心)
├── Air780EGNetwork (网络管理)
├── Air780EGGNSS (GNSS定位)
└── Air780EGDebug (调试系统)
```

## 2. 现有代码分析

根据你的项目结构，需要迁移的文件包括：

### 需要替换的文件
- `src/net/Air780EGModem.cpp/h` → 库的 `Air780EGCore`
- `src/net/Air780EGMqtt.cpp/h` → 待实现的 `Air780EGMqtt`
- `src/gps/Air780EGGNSSAdapter` → 库的 `Air780EGGNSS`

### 保留的文件
- `src/net/MqttManager.cpp/h` - 可以继续使用，但建议适配新库
- 其他GPS相关接口类 - 保持不变

## 3. 迁移步骤

### 步骤1：包含新库
在你的主文件中添加：
```cpp
#include <Air780EG.h>

// 创建全局实例
Air780EG air780;
```

### 步骤2：初始化配置
在 `setup()` 函数中：
```cpp
void setup() {
    // 设置调试级别
    Air780EG::setLogLevel(AIR780EG_LOG_INFO);
    Air780EG::setLogOutput(&Serial);
    
    // 初始化模块 (根据你的硬件配置调整)
    if (!air780.begin(&Serial1, 115200)) {
        Serial.println("Failed to initialize Air780EG");
        return;
    }
    
    // 启用网络
    if (air780.getNetwork().enableNetwork()) {
        air780.getNetwork().setUpdateInterval(5000); // 5秒更新
    }
    
    // 启用GNSS
    if (air780.getGNSS().enableGNSS()) {
        air780.getGNSS().setUpdateFrequency(1.0); // 1Hz
    }
}
```

### 步骤3：主循环维护
在 `loop()` 函数中：
```cpp
void loop() {
    // 必须调用，维护所有子模块
    air780.loop();
    
    // 你的其他代码...
}
```

### 步骤4：替换现有调用

#### 网络状态查询
**旧代码：**
```cpp
// 直接发送AT指令
String response = modem.sendATCommand("AT+CREG?");
// 解析响应...
```

**新代码：**
```cpp
// 使用缓存的数据，无AT指令调用
if (air780.getNetwork().isNetworkRegistered()) {
    int signal = air780.getNetwork().getSignalStrength();
    String operator_name = air780.getNetwork().getOperatorName();
}
```

#### GNSS数据获取
**旧代码：**
```cpp
// 直接发送AT指令
String response = modem.sendATCommand("AT+CGNSINF");
// 解析GNSS数据...
```

**新代码：**
```cpp
// 使用缓存的数据，无AT指令调用
if (air780.getGNSS().isFixed()) {
    double lat = air780.getGNSS().getLatitude();
    double lng = air780.getGNSS().getLongitude();
    int satellites = air780.getGNSS().getSatelliteCount();
}
```

## 4. 适配现有的GPSManager

你可以创建一个适配器来桥接新库和现有的GPSManager：

```cpp
// 在 src/gps/ 目录下创建 Air780EGLibAdapter.h
class Air780EGLibAdapter : public GPSInterface {
private:
    Air780EG* air780_instance;
    
public:
    Air780EGLibAdapter(Air780EG* instance) : air780_instance(instance) {}
    
    virtual bool begin() override {
        return air780_instance && air780_instance->getGNSS().enableGNSS();
    }
    
    virtual void loop() override {
        // 由主循环的 air780.loop() 处理
    }
    
    virtual bool isReady() override {
        return air780_instance && air780_instance->getGNSS().isFixed();
    }
    
    virtual gps_data_t getGPSData() const override {
        gps_data_t data = {};
        if (air780_instance && air780_instance->getGNSS().isDataValid()) {
            data.latitude = air780_instance->getGNSS().getLatitude();
            data.longitude = air780_instance->getGNSS().getLongitude();
            data.altitude = air780_instance->getGNSS().getAltitude();
            data.speed = air780_instance->getGNSS().getSpeed();
            data.satellites = air780_instance->getGNSS().getSatelliteCount();
            data.is_valid = air780_instance->getGNSS().isFixed();
        }
        return data;
    }
};
```

## 5. 配置文件更新

在 `config.h` 中添加库相关配置：

```cpp
// Air780EG库配置
#define AIR780EG_SERIAL_PORT        Serial1
#define AIR780EG_BAUDRATE          115200
#define AIR780EG_RESET_PIN         -1      // 如果有硬件复位引脚
#define AIR780EG_POWER_PIN         -1      // 如果有电源控制引脚

// 更新频率配置
#define AIR780EG_NETWORK_UPDATE_INTERVAL    5000    // 网络状态更新间隔(ms)
#define AIR780EG_GNSS_UPDATE_FREQUENCY      1.0     // GNSS更新频率(Hz)

// 调试配置
#define AIR780EG_LOG_LEVEL         AIR780EG_LOG_INFO
```

## 6. 性能优化建议

### 减少AT指令调用
新库通过缓存机制大幅减少AT指令调用：
- 网络状态：从每次查询变为定时更新
- GNSS数据：从每次查询变为按频率更新
- 所有get方法都返回缓存数据

### 配置合适的更新频率
```cpp
// 根据应用需求调整更新频率
air780.getNetwork().setUpdateInterval(10000);  // 网络状态10秒更新一次
air780.getGNSS().setUpdateFrequency(0.5);      // GNSS 0.5Hz (2秒一次)
```

## 7. 调试和故障排除

### 启用详细调试
```cpp
Air780EG::setLogLevel(AIR780EG_LOG_DEBUG);  // 或 AIR780EG_LOG_VERBOSE
```

### 常见问题检查
1. **串口配置**：确认串口号和波特率正确
2. **电源供应**：确保模块电源充足
3. **天线连接**：检查GNSS天线连接
4. **SIM卡**：确认SIM卡正确插入

### 状态监控
```cpp
// 定期显示详细状态
air780.printStatus();
air780.getNetwork().printNetworkInfo();
air780.getGNSS().printGNSSInfo();
```

## 8. 渐进式迁移策略

建议采用渐进式迁移：

1. **第一阶段**：保留现有代码，并行运行新库进行测试
2. **第二阶段**：逐步替换网络相关功能
3. **第三阶段**：替换GNSS相关功能
4. **第四阶段**：清理旧代码，完全使用新库

## 9. 测试验证

使用提供的测试程序验证库功能：
```bash
# 编译测试程序
pio run -t upload

# 监控串口输出
python monitor_serial.py
```

## 10. 后续扩展

新库为后续功能扩展预留了接口：
- MQTT客户端功能
- LBS基站定位
- 短信功能
- 语音通话功能

通过这种分层设计，可以轻松添加新功能而不影响现有代码。
