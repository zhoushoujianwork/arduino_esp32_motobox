# Compass 引脚问题解决方案

## 问题描述
用户报告compass组件航向角一直没有变化，显示磁场强度都是0（X=0.00 Y=0.00 Z=0.00），航向角固定在353.50°。

## 问题分析

### 1. 引脚配置问题
**原始配置：**
- Compass使用：`GPS_COMPASS_SDA`, `GPS_COMPASS_SCL`
- 在platformio.ini中定义为：`GPS_COMPASS_SDA=13`, `GPS_COMPASS_SCL=12`

**IMU配置：**
- IMU使用：`IMU_SDA_PIN=33`, `IMU_SCL_PIN=32`

### 2. I2C总线问题
**原始配置：**
- Compass使用：`Wire` (I2C0)
- IMU使用：`Wire1` (I2C1)

**问题：**
在同一PCB上，compass和IMU通常共享同一个I2C总线，使用不同的Wire对象会导致初始化冲突。

## 解决方案

### 1. 统一引脚配置
修改compass使用与IMU相同的I2C引脚：

```cpp
#ifdef ENABLE_COMPASS
// 使用与IMU相同的I2C引脚
#ifdef IMU_SDA_PIN
Compass compass(IMU_SDA_PIN, IMU_SCL_PIN);
#else
// 如果没有定义IMU引脚，使用GPS_COMPASS引脚作为备选
#ifdef GPS_COMPASS_SDA
Compass compass(GPS_COMPASS_SDA, GPS_COMPASS_SCL);
#else
// 默认引脚
Compass compass(33, 32);
#endif
#endif
#endif
```

### 2. 统一I2C总线
修改compass使用与IMU相同的Wire1总线：

```cpp
// 在Compass.cpp中
Compass::Compass(int sda, int scl) : _wire(Wire1) {
    // ...
}
```

### 3. 当前有效配置
修改后的配置：
- **引脚：** SDA=33, SCL=32（与IMU共享）
- **I2C总线：** Wire1（与IMU共享）
- **地址：** QMC5883L默认地址（0x0D）

## 验证方法

### 1. 检查初始化日志
正常情况下应该看到：
```
[罗盘] 初始化: SDA=33, SCL=32, 磁偏角=-6.50
[罗盘] Wire.begin() 成功
[罗盘] 初始化完成
```

### 2. 检查数据变化
正常情况下磁场强度应该不为0：
```
[罗盘] 航向: 123.45° (2.154 rad), 方向: SE (Southeast, 东南), 磁场: X=123.4 Y=-456.7 Z=789.0
```

### 3. 物理测试
旋转设备，航向角应该相应变化。

## 可能的其他问题

### 1. 硬件连接
- 检查SDA/SCL引脚是否正确连接
- 检查上拉电阻是否存在（通常4.7kΩ）
- 检查电源供应是否正常

### 2. I2C地址冲突
- QMC5883L默认地址：0x0D
- 确保没有其他设备使用相同地址

### 3. 电磁干扰
- 远离强磁场环境测试
- 检查PCB布局是否合理

## 调试命令

可以通过串口发送以下命令进行调试：

```
compass_status      - 显示罗盘状态
compass_test        - 运行功能测试
compass_calibrate   - 启动校准流程
compass_reset       - 重置罗盘
compass_declination -6.5  - 设置磁偏角
```

## 预期结果

修改后，compass应该能够：
1. 正确初始化
2. 读取到非零的磁场数据
3. 根据设备旋转输出变化的航向角
4. 正确识别8个主要方向（N, NE, E, SE, S, SW, W, NW）

## 注意事项

1. **首次使用需要校准**：在新环境中使用时建议进行校准
2. **磁偏角设置**：根据地理位置设置正确的磁偏角值
3. **避免干扰**：使用时远离强磁场和电子设备
4. **定期校准**：环境变化时可能需要重新校准
