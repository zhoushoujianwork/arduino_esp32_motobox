# Air780EG GNSS自动上报(URC)控制解决方案

## 问题描述

Air780EG模块在启用GNSS功能时，会自动发送GNSS定位信息的URC（未请求的结果代码）消息，格式如下：
```
+UGNSINF: 1,0,20250707212251,,,0.000,0.00,0.00,1,,99.9,99.9,4.00,,0,0,,,0,,
```

这些连续的URC消息可能会：
1. 占用串口带宽，影响MQTT通信
2. 增加系统处理负担
3. 导致AT命令队列拥堵

## 解决方案

### 1. 修改代码实现

#### 1.1 头文件修改 (Air780EGModem.h)
在GNSS功能部分添加新函数声明：
```cpp
// GNSS 功能
bool enableGNSS(bool enable);
bool enableGNSSAutoReport(bool enable);  // 控制GNSS自动上报 <-- 新增
bool isGNSSEnabled();
// ... 其他函数
```

#### 1.2 实现文件修改 (Air780EGModem.cpp)

**A. 修改GNSS禁用函数**
```cpp
} else {
    Serial.println("[Air780EG] 禁用GNSS");
    
    // 1. 首先关闭GNSS自动上报
    Serial.println("[Air780EG] 关闭GNSS自动上报...");
    sendAT("AT+CGNSURC=0", "OK", 3000);
    
    // 2. 然后关闭GNSS电源
    if (sendAT("AT+CGNSPWR=0", "OK", 3000)) {
        Serial.println("[Air780EG] GNSS已禁用");
        _gnssEnabled = false;
        return true;
    } else {
        Serial.println("[Air780EG] GNSS禁用失败");
        return false;
    }
}
```

**B. 添加独立的URC控制函数**
```cpp
// 控制GNSS自动上报
bool Air780EGModem::enableGNSSAutoReport(bool enable) {
    if (enable) {
        Serial.println("[Air780EG] 启用GNSS自动上报");
        if (sendAT("AT+CGNSURC=1", "OK", 3000)) {
            Serial.println("[Air780EG] GNSS自动上报已启用");
            return true;
        } else {
            Serial.println("[Air780EG] GNSS自动上报启用失败");
            return false;
        }
    } else {
        Serial.println("[Air780EG] 禁用GNSS自动上报");
        if (sendAT("AT+CGNSURC=0", "OK", 3000)) {
            Serial.println("[Air780EG] GNSS自动上报已禁用");
            return true;
        } else {
            Serial.println("[Air780EG] GNSS自动上报禁用失败");
            return false;
        }
    }
}
```

### 2. AT命令说明

| 命令 | 功能 | 参数 |
|------|------|------|
| `AT+CGNSURC=1` | 启用GNSS自动上报 | 1=启用 |
| `AT+CGNSURC=0` | 关闭GNSS自动上报 | 0=关闭 |
| `AT+CGNSURC?` | 查询当前设置 | 返回当前状态 |

### 3. 使用方法

#### 3.1 在代码中使用
```cpp
// 关闭GNSS自动上报（保持GNSS功能）
air780eg_modem.enableGNSSAutoReport(false);

// 重新启用GNSS自动上报
air780eg_modem.enableGNSSAutoReport(true);

// 完全禁用GNSS（会自动关闭URC）
air780eg_modem.enableGNSS(false);
```

#### 3.2 直接AT命令测试
```bash
# 使用Python测试脚本
python3 scripts/test_gnss_urc_control.py

# 或手动发送AT命令
AT+CGNSURC=0  # 关闭自动上报
AT+CGNSURC?   # 查询状态
```

### 4. 验证方法

#### 4.1 通过串口监控
1. 发送 `AT+CGNSURC=0` 关闭自动上报
2. 观察10-30秒，确认不再收到 `+UGNSINF:` 消息
3. 发送 `AT+CGNSURC=1` 重新启用
4. 确认 `+UGNSINF:` 消息恢复

#### 4.2 通过代码验证
```cpp
void testGNSSURCControl() {
    Serial.println("关闭GNSS URC...");
    if (air780eg_modem.enableGNSSAutoReport(false)) {
        Serial.println("✅ 关闭成功，等待10秒观察...");
        delay(10000);
        Serial.println("✅ 测试完成");
    }
}
```

### 5. 注意事项

1. **GNSS功能保持**：关闭URC不会影响GNSS定位功能，仍可通过 `AT+CGNSINF` 主动查询位置
2. **MQTT通信优化**：关闭URC后，MQTT通信应该更加稳定
3. **按需启用**：可以在需要实时位置更新时临时启用URC
4. **重启恢复**：模块重启后URC设置会恢复默认值，需要重新设置

### 6. 建议的使用策略

#### 6.1 启动时策略
```cpp
void setup() {
    // 初始化Air780EG
    air780eg_modem.begin();
    
    // 启用GNSS但关闭自动上报
    air780eg_modem.enableGNSS(true);
    delay(2000);
    air780eg_modem.enableGNSSAutoReport(false);
    
    Serial.println("GNSS已启用，URC已关闭");
}
```

#### 6.2 按需获取位置
```cpp
void getLocationWhenNeeded() {
    // 临时启用URC获取实时位置
    air780eg_modem.enableGNSSAutoReport(true);
    delay(5000);  // 等待几秒获取位置更新
    air780eg_modem.enableGNSSAutoReport(false);
    
    // 或直接查询当前位置
    GNSSData gnssData = air780eg_modem.getGNSSData();
    if (gnssData.valid) {
        Serial.printf("位置: %.6f, %.6f\n", gnssData.latitude, gnssData.longitude);
    }
}
```

### 7. 故障排除

| 问题 | 可能原因 | 解决方法 |
|------|----------|----------|
| URC仍在发送 | AT命令失败 | 检查串口通信，重试命令 |
| GNSS功能异常 | 命令顺序错误 | 先启用GNSS，再设置URC |
| 设置不生效 | 模块状态异常 | 重启模块，重新设置 |

## 总结

通过添加 `enableGNSSAutoReport()` 函数，现在可以灵活控制Air780EG的GNSS自动上报功能，在保持定位能力的同时优化MQTT通信性能。这个解决方案提供了：

1. ✅ 独立的URC控制功能
2. ✅ 保持GNSS定位能力
3. ✅ 优化MQTT通信稳定性
4. ✅ 灵活的使用策略
5. ✅ 完整的测试验证方法
