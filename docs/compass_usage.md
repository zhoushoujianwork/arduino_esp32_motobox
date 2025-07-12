# Compass 组件使用指南

## 概述
重构后的Compass组件提供了完整的罗盘功能，包括角度获取、方向判断（东南西北）等功能。

## 主要特性

### 1. 完整的方向信息
- 支持8个主要方向：N, NE, E, SE, S, SW, W, NW
- 提供英文、中文方向名称
- 角度范围：0-360度
- 支持弧度制

### 2. 数据结构
```cpp
typedef struct {
    float x, y, z;              // 磁场强度
    float heading;              // 航向角 0-360度
    float headingRadians;       // 航向角弧度
    CompassDirection direction; // 方向枚举
    const char* directionStr;   // 方向字符串 (N, NE, E, SE, S, SW, W, NW)
    const char* directionName;  // 方向全名 (North, Northeast, etc.)
    const char* directionCN;    // 中文方向名
    bool isValid;               // 数据是否有效
    unsigned long timestamp;    // 数据时间戳
} compass_data_t;
```

## 使用方法

### 1. 基本初始化
```cpp
#ifdef ENABLE_COMPASS
    if (compass.begin()) {
        Serial.println("罗盘初始化成功");
    } else {
        Serial.println("罗盘初始化失败");
    }
#endif
```

### 2. 在主循环中更新数据
```cpp
#ifdef ENABLE_COMPASS
    compass.loop();  // 自动更新全局compass_data
#endif
```

### 3. 获取方向信息
```cpp
// 方法1：直接访问全局数据
if (compass_data.isValid) {
    Serial.printf("当前方向: %s (%s, %s)\n", 
        compass_data.directionStr,
        compass_data.directionName, 
        compass_data.directionCN);
    Serial.printf("航向角: %.2f度\n", compass_data.heading);
}

// 方法2：通过类方法获取
float heading = compass.getHeading();
const char* direction = compass.getCurrentDirectionStr();
const char* directionCN = compass.getCurrentDirectionCN();
```

### 4. 工具函数使用
```cpp
// 根据角度获取方向
float angle = 45.5;
CompassDirection dir = getDirection(angle);
const char* dirStr = getDirectionStr(angle);
const char* dirName = getDirectionName(angle);
const char* dirCN = getDirectionCN(angle);

// 角度标准化
float normalized = normalizeHeading(-30);  // 结果: 330
```

### 5. 校准功能
```cpp
// 启动校准（需要手动旋转设备）
if (compass.calibrate()) {
    Serial.println("校准完成，请记录校准参数");
}

// 设置已知的校准参数
compass.setCalibration(-123, 456, -789, 1.05, 0.95, 1.02);
```

### 6. 高级功能
```cpp
// 设置磁偏角（根据地理位置调整）
compass.setDeclination(-6.5);  // 中国大部分地区约为-6.5度

// 检查状态
if (compass.isInitialized() && compass.isDataValid()) {
    // 使用罗盘数据
}

// 重置罗盘
compass.reset();
```

## 方向枚举说明
```cpp
enum CompassDirection {
    NORTH = 0,      // 北    (337.5° - 22.5°)
    NORTHEAST,      // 东北  (22.5° - 67.5°)
    EAST,           // 东    (67.5° - 112.5°)
    SOUTHEAST,      // 东南  (112.5° - 157.5°)
    SOUTH,          // 南    (157.5° - 202.5°)
    SOUTHWEST,      // 西南  (202.5° - 247.5°)
    WEST,           // 西    (247.5° - 292.5°)
    NORTHWEST       // 西北  (292.5° - 337.5°)
};
```

## 调试功能
```cpp
// 启用/禁用调试输出
compass.setDebug(true);

// 打印当前罗盘数据
printCompassData();
```

## 注意事项

1. **磁偏角校正**：根据使用地点设置正确的磁偏角值
2. **校准重要性**：首次使用或更换环境时需要重新校准
3. **电磁干扰**：避免在强磁场环境中使用
4. **数据有效性**：使用前检查 `compass_data.isValid` 标志

## 示例代码
```cpp
void handleCompassData() {
    if (!compass_data.isValid) {
        Serial.println("罗盘数据无效");
        return;
    }
    
    // 获取当前方向
    float heading = compass_data.heading;
    const char* direction = compass_data.directionStr;
    const char* directionCN = compass_data.directionCN;
    
    // 根据方向执行不同操作
    switch (compass_data.direction) {
        case NORTH:
            Serial.println("正在朝北行驶");
            break;
        case EAST:
            Serial.println("正在朝东行驶");
            break;
        case SOUTH:
            Serial.println("正在朝南行驶");
            break;
        case WEST:
            Serial.println("正在朝西行驶");
            break;
        default:
            Serial.printf("正在朝%s行驶\n", directionCN);
            break;
    }
    
    // 精确角度判断
    if (heading >= 350 || heading <= 10) {
        Serial.println("几乎正北方向");
    }
}
```
