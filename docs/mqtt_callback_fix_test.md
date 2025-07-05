# MQTT 回调修复测试文档

## 问题描述
Air780EG 模块能够正确接收 MQTT URC 消息，但由于没有设置消息回调函数，导致消息被丢弃：

```
[Air780EG] 收到MQTT URC: +MSUB: "vehicle/v1/ECE334B0AD08/ctrl/",14 byte,7B22636D64223A2022313233227D
[Air780EG] ❌ MQTT消息回调未设置，消息将被丢弃
```

## 修复方案

### 1. 添加 MQTT 消息处理函数
在 `main.cpp` 中添加了 `handleMQTTMessage` 函数：

```cpp
void handleMQTTMessage(String topic, String payload) {
  MQTT_DEBUG_PRINTLN("[MQTT] 收到消息: " + topic + " -> " + payload);
  
  // 解析JSON格式的控制命令
  if (topic.indexOf("/ctrl/") >= 0 || topic.indexOf("/control") >= 0) {
    // 处理控制命令
    // 支持 {"cmd": "command"} 格式的JSON消息
  }
  else if (topic.indexOf("/config") >= 0) {
    // 处理配置命令
  }
}
```

### 2. 设置回调函数
在 `setup()` 函数中设置 MQTT 回调：

```cpp
#ifdef USE_AIR780EG_GSM
  Serial.println("[Air780EG] 设置MQTT消息回调");
  air780eg_modem.setMQTTCallback(handleMQTTMessage);
#endif
```

## 测试步骤

### 1. 编译和上传固件
确保启用了以下配置：
- `USE_AIR780EG_GSM` 已定义
- `MQTT_DEBUG_ENABLED` 设置为 `true`

### 2. 监控串口输出
上传固件后，监控串口输出，应该看到：
```
[Air780EG] 设置MQTT消息回调
```

### 3. 发送测试消息
通过 MQTT 客户端向设备订阅的主题发送消息，例如：

**主题**: `vehicle/v1/ECE334B0AD08/ctrl/`
**消息**: `{"cmd": "status"}`

### 4. 验证消息处理
如果修复成功，应该看到类似的输出：
```
[Air780EG] 收到MQTT URC: +MSUB: "vehicle/v1/ECE334B0AD08/ctrl/",14 byte,7B22636D64223A2022737461747573227D
[Air780EG] 开始解析MQTT消息内容...
[Air780EG] 提取消息部分:  "vehicle/v1/ECE334B0AD08/ctrl/",14 byte,7B22636D64223A2022737461747573227D
[Air780EG] 解析到主题: 'vehicle/v1/ECE334B0AD08/ctrl/'
[Air780EG] 预期消息长度: 14
[Air780EG] 解析到十六进制消息: '7B22636D64223A2022737461747573227D'
[Air780EG] 转换后的消息: '{"cmd": "status"}'
[Air780EG] ✅ MQTT消息解析成功，调用回调函数
[MQTT] 收到消息: vehicle/v1/ECE334B0AD08/ctrl/ -> {"cmd": "status"}
[MQTT] 处理控制命令: {"cmd": "status"}
[MQTT] 提取到命令: status
[MQTT] 执行状态查询命令
```

## 支持的命令格式

### JSON 格式
```json
{"cmd": "command_name"}
```

### 支持的命令
- `status` - 查询设备状态
- `reboot` - 重启设备
- `info` - 查询设备信息

### 示例消息

1. **状态查询**
   - 主题: `vehicle/v1/ECE334B0AD08/ctrl/`
   - 消息: `{"cmd": "status"}`

2. **重启设备**
   - 主题: `vehicle/v1/ECE334B0AD08/ctrl/`
   - 消息: `{"cmd": "reboot"}`

3. **设备信息**
   - 主题: `vehicle/v1/ECE334B0AD08/ctrl/`
   - 消息: `{"cmd": "info"}`

## 调试技巧

### 1. 启用调试输出
确保在 `config.h` 中设置：
```cpp
#define MQTT_DEBUG_ENABLED true
```

### 2. 检查回调设置
在串口输出中查找：
```
[Air780EG] 设置MQTT消息回调
[Air780EG: MQTT回调函数已设置
```

### 3. 监控消息解析过程
完整的消息处理过程包括：
1. URC 消息接收
2. 消息格式解析
3. 十六进制转换
4. 回调函数调用
5. 命令处理

### 4. 常见问题排查

**问题**: 仍然显示"MQTT消息回调未设置"
**解决**: 检查 `USE_AIR780EG_GSM` 是否正确定义，确保回调设置代码被执行

**问题**: 消息解析失败
**解决**: 检查消息格式是否正确，确保是有效的十六进制字符串

**问题**: 命令不被识别
**解决**: 检查 JSON 格式是否正确，确保包含 `"cmd"` 字段

## 扩展功能

可以在 `handleMQTTMessage` 函数中添加更多功能：

1. **配置管理**: 处理设备配置更新
2. **状态上报**: 响应状态查询命令
3. **远程控制**: 实现更多控制命令
4. **数据采集**: 响应数据采集请求

## 注意事项

1. **内存管理**: MQTT 消息处理在回调函数中进行，避免长时间阻塞操作
2. **错误处理**: 添加了异常捕获，防止解析错误导致系统崩溃
3. **调试输出**: 详细的调试信息有助于问题排查，但会增加串口输出量
4. **安全考虑**: 在生产环境中应该添加消息验证和权限检查
