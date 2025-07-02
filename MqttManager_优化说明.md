# MqttManager 优化说明

## 问题描述
原有的 `publishToTopic` 方法在 WiFi 和 4G 模式下的连接状态检查逻辑不统一，导致在某些情况下发布失败但没有正确处理连接状态。

## 主要优化内容

### 1. 统一连接状态检查逻辑
- **优化前**: WiFi 和 4G 模式使用不同的连接检查逻辑，代码重复且容易出错
- **优化后**: 使用统一的 `isNetworkConnected()` 和 `isMqttConnected()` 方法

```cpp
// 统一的连接状态检查
if (!isNetworkConnected() || !isMqttConnected()) {
    debugPrint("MqttManager 网络或MQTT未连接");
    setMqttState(MqttState::DISCONNECTED);
    return false;
}
```

### 2. 修复网络状态检查方法
- **修复前**: 4G 模式下使用 `ml307Mqtt.canPublish()` 检查网络状态
- **修复后**: 使用 `ml307_at.isNetworkReady()` 检查网络基础连接状态

```cpp
bool MqttManager::isNetworkConnected() const {
#ifdef ENABLE_WIFI
    return WiFi.status() == WL_CONNECTED;
#else
    return ml307_at.isNetworkReady();  // 修复：使用正确的网络状态检查
#endif
}
```

### 3. 增强错误处理机制
- 添加详细的调试信息，包括连接状态和错误原因
- 发布失败时进行连接状态重新验证
- 根据不同的失败原因设置相应的 MQTT 状态

```cpp
// 发布失败时的详细错误处理
if (!result) {
    debugPrint("MqttManager 发布失败 - 主题: " + it->second.topic);
    
    if (!isNetworkConnected()) {
        debugPrint("MqttManager 网络连接已断开");
        setMqttState(MqttState::DISCONNECTED);
    } else if (!isMqttConnected()) {
        debugPrint("MqttManager MQTT连接已断开，尝试强制重连");
        setMqttState(MqttState::CONNECTING);
        // 对于重要主题，尝试强制重连
        if (name == "device_info" || name == "gps") {
            forceReconnect();
        }
    } else {
        debugPrint("MqttManager 发布失败但连接正常，可能是临时网络问题");
        setMqttState(MqttState::ERROR);
    }
}
```

### 4. 添加强制重连机制
- 新增 `forceReconnect()` 方法，用于主动断开并重新建立连接
- 对重要主题（如 device_info、gps）发布失败时触发强制重连

```cpp
bool MqttManager::forceReconnect() {
    debugPrint("MqttManager 强制重新连接");
    
    // 先断开现有连接
#ifdef ENABLE_WIFI
    if (_wifiMqttClient && _wifiMqttClient->connected()) {
        _wifiMqttClient->disconnect();
        delay(100);
    }
#else
    if (ml307Mqtt.connected()) {
        ml307Mqtt.disconnect();
        delay(500); // 4G模块需要更长时间断开
    }
#endif

    // 重置连接状态并尝试重新连接
    setMqttState(MqttState::CONNECTING);
    _lastReconnectAttempt = millis();
    _connectionStartTime = millis();
    
    return connect();
}
```

### 5. 优化 publish 方法
- 添加参数有效性检查
- 发布前再次验证连接状态
- 增加更详细的错误日志，包括 MQTT 客户端状态

```cpp
bool MqttManager::publish(const char *topic, const char *payload, bool retain) {
    if (!topic || !payload) {
        debugPrint("MQTT publish 参数无效 - topic或payload为空");
        return false;
    }

    // 发布前再次检查连接状态
    if (!isNetworkConnected()) {
        debugPrint("MQTT publish 失败 - 网络未连接");
        setMqttState(MqttState::DISCONNECTED);
        return false;
    }
    
    if (!isMqttConnected()) {
        debugPrint("MQTT publish 失败 - MQTT未连接");
        setMqttState(MqttState::CONNECTING);
        return false;
    }

    // ... 执行发布操作
}
```

### 6. 统一 WiFi 模式的连接状态检查
- 使用与 4G 模式相同的统一方法检查连接状态
- 简化状态设置逻辑

```cpp
// WiFi逻辑优化
bool currentConnected = isNetworkConnected() && isMqttConnected();

// 根据网络和MQTT连接状态设置相应的状态
if (!isNetworkConnected()) {
    setMqttState(MqttState::DISCONNECTED);
} else if (!isMqttConnected()) {
    setMqttState(MqttState::CONNECTING);
    reconnect();
} else {
    setMqttState(MqttState::CONNECTED);
}
```

## 优化效果

1. **统一性**: WiFi 和 4G 模式使用相同的连接检查逻辑，减少代码重复
2. **可靠性**: 增强的错误处理和重连机制提高了连接的稳定性
3. **可维护性**: 清晰的错误日志和状态管理便于问题排查
4. **健壮性**: 强制重连机制确保重要数据能够及时发布

## 注意事项

1. 强制重连会有短暂的连接中断，仅在必要时使用
2. 4G 模块的断开操作需要更长的等待时间（500ms）
3. 重要主题（device_info、gps）会触发强制重连，确保数据传输的可靠性
4. 调试模式下会输出详细的连接状态和错误信息，便于问题定位

## 测试建议

1. 测试 WiFi 模式下的网络断开和恢复场景
2. 测试 4G 模式下的网络信号变化场景
3. 测试 MQTT 服务器断开和恢复场景
4. 验证重要主题的强制重连机制
5. 检查调试日志的完整性和准确性
