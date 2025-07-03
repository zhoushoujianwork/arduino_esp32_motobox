# Air780EG 4G+GNSS模块使用说明

## 概述

Air780EG模块集成了4G Cat.1通信和GNSS定位功能，为ESP32-S3 MotoBox项目提供移动网络连接和精确定位能力。

## 硬件连接

### 引脚配置
```cpp
// platformio.ini 中的配置
-D GSM_RX_PIN=25    // ESP32接收，连接Air780EG的TX
-D GSM_TX_PIN=26    // ESP32发送，连接Air780EG的RX  
-D GSM_EN=34        // 电源使能控制引脚
-D ENABLE_AIR780EG  // 启用Air780EG功能
```

### 硬件连接图
```
ESP32-S3        Air780EG开发板
GPIO25    <-->  TX
GPIO26    <-->  RX
GPIO34    <-->  PWR_KEY
GND       <-->  GND
3.3V      <-->  VCC
```

## 软件使用

### 1. 基础初始化

```cpp
#include "net/Air780EGModem.h"
#include "net/Air780EGMqtt.h"

// 全局实例已在Air780EGModem.cpp中定义
// Air780EGModem air780eg_modem(Serial2, GSM_RX_PIN, GSM_TX_PIN, GSM_EN);

void setup() {
    Serial.begin(115200);
    
    // 初始化Air780EG模块
    air780eg_modem.setDebug(true);
    if (air780eg_modem.begin()) {
        Serial.println("Air780EG初始化成功");
        
        // 等待网络注册
        while (!air780eg_modem.isNetworkReady()) {
            Serial.println("等待网络注册...");
            delay(2000);
        }
        
        // 显示网络信息
        air780eg_modem.debugNetworkInfo();
    } else {
        Serial.println("Air780EG初始化失败");
    }
}
```

### 2. GNSS定位功能

```cpp
void setupGNSS() {
    // 启用GNSS
    if (air780eg_modem.enableGNSS(true)) {
        Serial.println("GNSS启用成功");
        
        // 设置更新频率为5Hz
        air780eg_modem.setGNSSUpdateRate(5);
    }
}

void loopGNSS() {
    // 更新GNSS数据
    if (air780eg_modem.updateGNSSData()) {
        GNSSData gnss = air780eg_modem.getGNSSData();
        
        if (gnss.valid) {
            Serial.printf("位置: %.6f, %.6f\n", gnss.latitude, gnss.longitude);
            Serial.printf("海拔: %.2fm, 速度: %.2fkm/h\n", gnss.altitude, gnss.speed);
            Serial.printf("卫星数: %d, HDOP: %.2f\n", gnss.satellites, gnss.hdop);
        }
    }
    
    // 或者使用兼容的GPS接口
    gps_data_t gps = air780eg_modem.getGPSData();
    if (gps.valid) {
        // 与现有GPS数据处理代码兼容
        // 可以直接替换现有的GPS数据源
    }
}
```

### 3. MQTT通信

```cpp
Air780EGMqtt mqtt(air780eg_modem);

void setupMQTT() {
    mqtt.setDebug(true);
    
    if (mqtt.begin()) {
        // 连接MQTT服务器
        if (mqtt.connect("your-mqtt-server.com", 1883, "motobox-001", "username", "password")) {
            Serial.println("MQTT连接成功");
            
            // 订阅控制主题
            mqtt.subscribe("motobox/control", 1);
            
            // 设置消息回调
            mqtt.setCallback([](String topic, String payload) {
                Serial.println("收到MQTT消息: " + topic + " -> " + payload);
                // 处理控制命令
                handleMQTTMessage(topic, payload);
            });
        }
    }
}

void loopMQTT() {
    if (mqtt.isConnected()) {
        // 处理接收消息
        mqtt.loop();
        
        // 发布状态数据
        static unsigned long lastPublish = 0;
        if (millis() - lastPublish > 30000) {  // 每30秒发布一次
            String status = createStatusJSON();
            mqtt.publish("motobox/status", status, 1);
            lastPublish = millis();
        }
        
        // 发布GPS数据
        if (air780eg_modem.isGNSSDataValid()) {
            GNSSData gnss = air780eg_modem.getGNSSData();
            String gpsJson = createGPSJSON(gnss);
            mqtt.publish("motobox/gps", gpsJson, 0);
        }
    }
}
```

### 4. 与现有系统集成

```cpp
// 在main.cpp中集成Air780EG
void setup() {
    // ... 其他初始化代码 ...
    
    #ifdef ENABLE_AIR780EG
    // 初始化Air780EG
    setupAir780EG();
    #endif
}

void loop() {
    // ... 其他循环代码 ...
    
    #ifdef ENABLE_AIR780EG
    // Air780EG数据处理
    loopAir780EG();
    #endif
}

void setupAir780EG() {
    air780eg_modem.setDebug(true);
    if (air780eg_modem.begin()) {
        Serial.println("Air780EG模块初始化成功");
        
        // 启用GNSS
        air780eg_modem.enableGNSS(true);
        
        // 初始化MQTT
        setupMQTT();
    }
}

void loopAir780EG() {
    // 更新GNSS数据
    air780eg_modem.updateGNSSData();
    
    // 处理MQTT通信
    loopMQTT();
    
    // 与SD卡记录集成
    if (air780eg_modem.isGNSSDataValid()) {
        gps_data_t gps = air780eg_modem.getGPSData();
        // 使用现有的GPS数据记录功能
        // sdManager.recordGPSData(gps);  // 假设有这个函数
    }
}
```

## 功能特性

### 网络功能
- 4G Cat.1网络连接
- 支持中国主要运营商
- 自动网络注册和重连
- 网络状态监控

### GNSS定位功能
- GPS + 北斗双模定位
- 64通道同时跟踪
- 可配置更新频率 (1/2/5/10Hz)
- 支持AGNSS辅助定位
- 兼容现有GPS数据接口

### MQTT通信
- 基于4G网络的MQTT通信
- 支持QoS 0/1/2
- 消息发布和订阅
- 自动重连机制

### LBS定位 (备用)
- 基站定位功能
- 作为GNSS的备用定位方案
- 适用于室内或GNSS信号弱的环境

## 调试功能

```cpp
// 网络信息调试
air780eg_modem.debugNetworkInfo();

// GNSS信息调试
air780eg_modem.debugGNSSInfo();

// LBS配置调试
air780eg_modem.debugLBSConfig();
```

## 错误处理

模块包含完善的错误处理机制：
- 网络连接失败自动重试
- MQTT连接断开自动重连
- CME错误退避算法
- 线程安全的AT命令执行

## 性能优化

- 避免频繁的网络状态检查
- GNSS数据更新频率控制
- LBS请求频率限制
- 内存使用优化

## 与ML307的区别

| 功能 | ML307 | Air780EG |
|------|-------|----------|
| 网络 | 4G Cat.1 | 4G Cat.1 |
| 定位 | LBS | GNSS + LBS |
| 精度 | 基站级别 | 米级精度 |
| 功耗 | 中等 | 较低 |
| 成本 | 较低 | 中等 |

## 注意事项

1. **天线设计**: GNSS天线需要专门的PCB设计考虑
2. **供电要求**: 模块需要稳定的3.3V供电
3. **启动时间**: GNSS冷启动可能需要30-60秒
4. **室内定位**: GNSS在室内信号弱，可使用LBS作为备用
5. **数据流量**: 注意4G数据流量的使用

## 故障排除

### 常见问题

1. **模块无响应**
   - 检查供电和使能引脚
   - 检查串口连接和波特率
   - 尝试硬件重启

2. **网络注册失败**
   - 检查SIM卡安装
   - 确认运营商网络覆盖
   - 检查天线连接

3. **GNSS无定位**
   - 确保在室外开阔环境
   - 检查GNSS天线连接
   - 等待足够的搜星时间

4. **MQTT连接失败**
   - 确认网络连接正常
   - 检查服务器地址和端口
   - 验证用户名密码

通过以上配置和使用方法，Air780EG模块可以为ESP32-S3 MotoBox项目提供强大的移动网络连接和精确定位功能。
