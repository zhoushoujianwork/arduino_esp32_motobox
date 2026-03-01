#include "compass/Compass.h"

// TAG
static const char *TAG = "Compass";


#ifdef ENABLE_COMPASS
// 使用共享I2C管理器，不需要指定引脚
Compass compass;

// 初始化全局罗盘数据
compass_data_t compass_data = {
    .x = 0,
    .y = 0,
    .z = 0,
    .heading = 0,
    .headingRadians = 0,
    .direction = NORTH,
    .directionStr = "N",
    .directionName = "North",
    .directionCN = "北",
    .isValid = false,
    .timestamp = 0
};

// 方向字符串数组
static const char* DIRECTION_STRS[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
static const char* DIRECTION_NAMES[] = {"North", "Northeast", "East", "Southeast", 
                                       "South", "Southwest", "West", "Northwest"};
static const char* DIRECTION_CN[] = {"北", "东北", "东", "东南", "南", "西南", "西", "西北"};

/**
 * @brief 工具函数实现
 */
float normalizeHeading(float heading) {
    while (heading < 0) heading += 360;
    while (heading >= 360) heading -= 360;
    return heading;
}

CompassDirection getDirection(float heading) {
    heading = normalizeHeading(heading);
    int direction = (int)((heading + 22.5) / 45.0);
    if (direction >= 8) direction = 0;
    return (CompassDirection)direction;
}

const char* getDirectionStr(float heading) {
    CompassDirection dir = getDirection(heading);
    return DIRECTION_STRS[dir];
}

const char* getDirectionName(float heading) {
    CompassDirection dir = getDirection(heading);
    return DIRECTION_NAMES[dir];
}

const char* getDirectionCN(float heading) {
    CompassDirection dir = getDirection(heading);
    return DIRECTION_CN[dir];
}

void printCompassData() {
    if (!compass_data.isValid) {
        Serial.printf("[%s] 数据无效\n", TAG);
        return;
    }
    
    Serial.printf("[%s] 航向: %.2f° (%.3f rad), 方向: %s (%s, %s), 磁场: X=%.2f Y=%.2f Z=%.2f\n", 
        TAG,
        compass_data.heading, 
        compass_data.headingRadians,
        compass_data.directionStr, 
        compass_data.directionName,
        compass_data.directionCN,
        compass_data.x, 
        compass_data.y, 
        compass_data.z);
}

/**
 * @brief QMC5883L 罗盘传感器驱动实现
 */
Compass::Compass() {
    _declination = -6.5f;  // 默认磁偏角，需要根据地理位置调整
    _initialized = false;
    _lastReadTime = 0;  
    _lastDebugPrintTime = 0;
    _useIndependentI2C = false;
    _wire = nullptr;
}

// 地磁使用独立的I2C总线初始化
bool Compass::begin() {
    Serial.printf("[%s] 初始化指南针，磁偏角=%.2f°\n", TAG, _declination);
    
#ifdef IIC_SDA_GY
#ifdef IIC_SCL_GY
    // 如果定义了地磁独立I2C引脚，使用独立的I2C总线（Wire）
    Serial.printf("[%s] 使用独立I2C总线 - SDA:%d, SCL:%d\n", TAG, IIC_SDA_GY, IIC_SCL_GY);
    _useIndependentI2C = true;
    _wire = &Wire;
    
    // 初始化独立的I2C总线
    if (!_wire->begin(IIC_SDA_GY, IIC_SCL_GY)) {
        Serial.printf("[%s] ❌ 独立I2C总线初始化失败\n", TAG);
        return false;
    }
    _wire->setClock(50000);  // 设置I2C频率为50kHz
    Serial.printf("[%s] ✅ 独立I2C总线初始化成功\n", TAG);
    
    delay(100);  // 给一些初始化时间
    
    // 扫描独立I2C总线上的设备
    Serial.printf("[%s] [调试] 扫描独立I2C总线上的设备...\n", TAG);
    byte error, address;
    int nDevices = 0;
    for(address = 1; address < 127; address++ ) {
        _wire->beginTransmission(address);
        error = _wire->endTransmission();
        if (error == 0) {
            Serial.printf("[%s] [调试] I2C设备发现于地址 0x%02X\n", TAG, address);
            nDevices++;
        }
    }
    if (nDevices == 0) {
        Serial.printf("[%s] [警告] 独立I2C总线上未发现任何设备！\n", TAG);
    } else {
        Serial.printf("[%s] [调试] 共发现 %d 个I2C设备\n", TAG, nDevices);
    }
    
    // 注意：QMC5883P可能使用不同的I2C地址（如0x2C），库会自动处理
    // 如果扫描到设备，说明I2C总线工作正常
    if (nDevices > 0) {
        Serial.printf("[%s] [调试] I2C总线工作正常，发现 %d 个设备\n", TAG, nDevices);
    }
    
    // 初始化QMC5883P传感器（使用独立I2C总线）
    // QMC5883P库使用全局Wire对象，需要确保Wire已经正确初始化
    // 注意：QMC5883P的begin()方法会调用_bus->begin()（无参数），
    // 所以我们需要确保Wire已经用正确的引脚初始化
    Serial.printf("[%s] 开始初始化QMC5883P传感器...\n", TAG);
    
    // 确保Wire已经用正确的引脚初始化（之前已经初始化过了）
    // QMC5883P的begin()方法内部会调用_bus->begin()，但不会传入引脚参数
    // 所以我们需要确保Wire已经正确配置
    delay(50); // 给I2C总线一些稳定时间
    
    if (!qmc.begin()) {
        Serial.printf("[%s] ❌ QMC5883P初始化失败！\n", TAG);
        Serial.printf("[%s] [调试] 检查I2C连接和地址...\n", TAG);
        // 尝试重新初始化Wire
        _wire->end();
        delay(10);
        _wire->begin(IIC_SDA_GY, IIC_SCL_GY);
        _wire->setClock(50000);
        delay(50);
        if (!qmc.begin()) {
            Serial.printf("[%s] ❌ QMC5883P初始化仍然失败！\n", TAG);
            return false;
        }
    }
    Serial.printf("[%s] ✅ QMC5883P初始化成功\n", TAG);
    
    // 测试读取一次数据，验证传感器是否正常工作
    delay(200);  // 增加延时，确保传感器稳定
    Serial.printf("[%s] [调试] 尝试读取QMC5883P数据...\n", TAG);
    float testXYZ[3];
    if (qmc.readXYZ(testXYZ)) {
        Serial.printf("[%s] 初始化后测试读取 - X:%.2f Y:%.2f Z:%.2f µT\n", TAG, testXYZ[0], testXYZ[1], testXYZ[2]);
        
        // 如果读取为0，尝试多次读取
        if (testXYZ[0] == 0 && testXYZ[1] == 0 && testXYZ[2] == 0) {
            Serial.printf("[%s] [警告] 首次读取为0，尝试多次读取...\n", TAG);
            for (int i = 0; i < 5; i++) {
                delay(100);
                if (qmc.readXYZ(testXYZ)) {
                    Serial.printf("[%s] [调试] 第%d次读取 - X:%.2f Y:%.2f Z:%.2f µT\n", TAG, i+1, testXYZ[0], testXYZ[1], testXYZ[2]);
                    if (testXYZ[0] != 0 || testXYZ[1] != 0 || testXYZ[2] != 0) {
                        break;
                    }
                }
            }
        }
    } else {
        Serial.printf("[%s] [警告] 无法读取QMC5883P数据\n", TAG);
    }
    
    _initialized = true;
    device_state.telemetry.modules.compass_ready = true;
    
    Serial.printf("[%s] ✅ 初始化完成\n", TAG);
    return true;
#else
    // IIC_SCL_GY未定义，使用共享I2C
    _useIndependentI2C = false;
#endif
#else
    // IIC_SDA_GY未定义，使用共享I2C
    _useIndependentI2C = false;
#endif

    // 如果没有定义独立I2C引脚，使用共享I2C管理器
    if (!_useIndependentI2C) {
        if (!I2CManager::getInstance().isInitialized()) {
            Serial.printf("[%s] ❌ 共享I2C未初始化，请先初始化I2C管理器!\n", TAG);
            return false;
        }
        Serial.printf("[%s] 使用共享I2C总线\n", TAG);
        _wire = nullptr;
        
        delay(100);  // 给一些初始化时间
        
        // 初始化QMC5883P传感器
        Serial.printf("[%s] 开始初始化QMC5883P传感器...\n", TAG);
        if (!qmc.begin()) {
            Serial.printf("[%s] ❌ QMC5883P初始化失败！\n", TAG);
            return false;
        }
        Serial.printf("[%s] ✅ QMC5883P初始化成功\n", TAG);
        
        // 测试读取一次数据，验证传感器是否正常工作
        delay(200);
        float testXYZ[3];
        if (qmc.readXYZ(testXYZ)) {
            Serial.printf("[%s] 初始化后测试读取 - X:%.2f Y:%.2f Z:%.2f µT\n", TAG, testXYZ[0], testXYZ[1], testXYZ[2]);
        } else {
            Serial.printf("[%s] [警告] 无法读取QMC5883P数据\n", TAG);
        }
        
        _initialized = true;
        device_state.telemetry.modules.compass_ready = true;
        
        Serial.printf("[%s] ✅ 初始化完成\n", TAG);
        return true;
    }
    
    return false;
}

bool Compass::update() {
    if (!_initialized) {
        return false;
    }

#ifdef IIC_SDA_GY
#ifdef IIC_SCL_GY
    // 重要：确保Wire使用独立I2C配置（9/10）
    // 因为IMU会重新配置Wire为共享I2C（17/16），
    // 所以每次读取前需要确保Wire使用正确的引脚
    if (_useIndependentI2C) {
        Wire.begin(IIC_SDA_GY, IIC_SCL_GY);
        Wire.setClock(50000);
    }
#endif
#endif

    // 注意：QMC5883P库内部管理I2C连接，不需要手动检查
    // 如果readXYZ()成功，说明连接正常

    // 使用QMC5883P库读取数据（单位：µT）
    float xyz[3];
    if (!qmc.readXYZ(xyz)) {
        // 没有新数据可用
        return false;
    }
    
    // 调试输出：显示原始读取的数据（更频繁）
    static unsigned long lastRawDataPrint = 0;
    if (millis() - lastRawDataPrint > 2000) { // 每2秒输出一次原始数据
        lastRawDataPrint = millis();
        Serial.printf("[%s] [调试] 原始数据 - X:%.2f Y:%.2f Z:%.2f µT\n", TAG, xyz[0], xyz[1], xyz[2]);
        
        // 如果数据一直为0，输出警告
        if (xyz[0] == 0 && xyz[1] == 0 && xyz[2] == 0) {
            Serial.printf("[%s] [警告] 数据持续为0，可能的问题：\n", TAG);
            Serial.printf("[%s] [警告] 1. 传感器未正确连接\n", TAG);
            Serial.printf("[%s] [警告] 2. I2C地址不正确\n", TAG);
            Serial.printf("[%s] [警告] 3. QMC5883P库未使用正确的I2C总线\n", TAG);
        }
    }
    
    // 使用库函数计算航向角（已包含磁偏角校正）
    float heading = qmc.getHeadingDeg(_declination);
    updateCompassData(xyz[0], xyz[1], xyz[2], heading);
    
    return true;
}

void Compass::loop() {
    if (!_initialized) {
        return;
    }

    // 更新数据
    update();

    // 定期打印调试信息（降低频率）
    // 无论是否定义 COMPASS_DEBUG_ENABLED，都输出基本信息
    if (millis() - _lastDebugPrintTime > 10000) { // 10秒输出一次
        _lastDebugPrintTime = millis();
        printCompassData();
        
        // 额外输出调试信息（使用float数据）
        Serial.printf("[%s] [调试] 原始值 - X:%.2f Y:%.2f Z:%.2f µT, 数据有效:%s, 初始化状态:%s\n", 
            TAG, 
            compass_data.x, compass_data.y, compass_data.z,
            compass_data.isValid ? "是" : "否",
            _initialized ? "已初始化" : "未初始化");
        
        // 如果使用独立I2C，显示I2C状态
        #ifdef IIC_SDA_GY
        #ifdef IIC_SCL_GY
        if (_useIndependentI2C) {
            Serial.printf("[%s] [调试] 使用独立I2C总线 - SDA:%d, SCL:%d\n", TAG, IIC_SDA_GY, IIC_SCL_GY);
        }
        #endif
        #endif
    }
}

float Compass::getHeading() {
    return compass_data.heading;
}

float Compass::getHeadingRadians() {
    return compass_data.headingRadians;
}

CompassDirection Compass::getCurrentDirection() {
    return compass_data.direction;
}

const char* Compass::getCurrentDirectionStr() {
    return compass_data.directionStr;
}

const char* Compass::getCurrentDirectionName() {
    return compass_data.directionName;
}

const char* Compass::getCurrentDirectionCN() {
    return compass_data.directionCN;
}

bool Compass::calibrate() {
    if (!_initialized) {
        Serial.printf("[%s] 罗盘未初始化，无法校准\n", TAG);
        return false;
    }
    
    Serial.printf("[%s] 开始校准，请旋转模块（水平8字运动）30秒...\n", TAG);
    Serial.printf("[%s] 注意：QMC5883P库需要手动校准，请参考库文档\n", TAG);
    
    // QMC5883P库使用setHardIronOffsets()方法设置硬铁偏移
    // 软铁校准需要在应用层实现
    float minX = 1e6, maxX = -1e6;
    float minY = 1e6, maxY = -1e6;
    unsigned long t0 = millis();
    
    while (millis() - t0 < 30000) { // 30秒校准
        float xyz[3];
        if (qmc.readXYZ(xyz)) {
            minX = min(minX, xyz[0]);
            maxX = max(maxX, xyz[0]);
            minY = min(minY, xyz[1]);
            maxY = max(maxY, xyz[1]);
        }
        delay(100);
    }
    
    float offX = (maxX + minX) / 2.0f;
    float offY = (maxY + minY) / 2.0f;
    float scaleX = (maxX - minX) / 2.0f;
    float scaleY = (maxY - minY) / 2.0f;
    float avg = (scaleX + scaleY) / 2.0f;
    
    Serial.printf("[%s] 校准完成！\n", TAG);
    Serial.printf("[%s] 偏移 X: %.3f µT, Y: %.3f µT\n", TAG, offX, offY);
    Serial.printf("[%s] 缩放 X: %.3f µT, Y: %.3f µT, 平均: %.3f µT\n", TAG, scaleX, scaleY, avg);
    Serial.printf("[%s] 请将以下代码添加到你的程序中：\n", TAG);
    Serial.printf("qmc.setHardIronOffsets(%.3ff, %.3ff);\n", offX, offY);
    Serial.printf("// 软铁校准需要在应用层实现\n");
    
    // 设置硬铁偏移
    qmc.setHardIronOffsets(offX, offY);
    
    return true;
}

void Compass::setCalibration(int xOffset, int yOffset, int zOffset, float xScale, float yScale, float zScale) {
    // QMC5883P库只支持硬铁偏移（X和Y），单位是µT
    qmc.setHardIronOffsets((float)xOffset, (float)yOffset);
    Serial.printf("[%s] 硬铁偏移已设置: X=%.2f µT, Y=%.2f µT\n", TAG, (float)xOffset, (float)yOffset);
    Serial.printf("[%s] 注意：软铁校准需要在应用层实现\n", TAG);
}

void Compass::getRawData(int16_t &x, int16_t &y, int16_t &z) {
    // QMC5883P库返回float类型（µT），转换为int16_t用于兼容
    float xyz[3];
    if (qmc.readXYZ(xyz)) {
        x = (int16_t)xyz[0];
        y = (int16_t)xyz[1];
        z = (int16_t)xyz[2];
    } else {
        // 如果读取失败，从compass_data获取
        x = (int16_t)compass_data.x;
        y = (int16_t)compass_data.y;
        z = (int16_t)compass_data.z;
    }
}

float Compass::getX() {
    return compass_data.x;
}

float Compass::getY() {
    return compass_data.y;
}

float Compass::getZ() {
    return compass_data.z;
}

void Compass::setDeclination(float declination) {
    _declination = declination;
    Serial.printf("[%s] 磁偏角设置为: %.2f°\n", TAG, _declination);
}

float Compass::getDeclination() {
    return _declination;
}

bool Compass::isInitialized() {
    return _initialized;
}

bool Compass::isDataValid() {
    return compass_data.isValid;
}

void Compass::reset() {
    _initialized = false;
    device_state.telemetry.modules.compass_ready = false;
    compass_data.isValid = false;
    Serial.printf("[%s] 罗盘已重置\n", TAG);
}

float Compass::calculateHeading(float x, float y) {
    float heading = atan2(y, x) * 180.0 / PI;
    heading += _declination;  // 应用磁偏角校正
    return normalizeHeading(heading);
}

void Compass::updateCompassData(float x, float y, float z, float heading) {
    compass_data.x = x;
    compass_data.y = y;
    compass_data.z = z;
    compass_data.heading = heading;
    compass_data.headingRadians = heading * PI / 180.0;
    compass_data.direction = getDirection(heading);
    compass_data.directionStr = DIRECTION_STRS[compass_data.direction];
    compass_data.directionName = DIRECTION_NAMES[compass_data.direction];
    compass_data.directionCN = DIRECTION_CN[compass_data.direction];
    compass_data.isValid = true;
    compass_data.timestamp = millis();
    
    // 更新到统一数据管理
    extern Device device;
    device.updateCompassData(heading, x, y, z);
}

void Compass::enterLowPowerMode()
{
    if (!_initialized) return;
    
    // QMC5883L进入待机模式
    // 通过设置控制寄存器进入低功耗模式
    TwoWire& wire = _useIndependentI2C ? *_wire : getSharedWire();
    wire.beginTransmission(0x0D); // QMC5883L地址
    wire.write(0x09); // 控制寄存器1
    wire.write(0x00); // 待机模式
    wire.endTransmission();
    
    Serial.printf("[%s] 进入低功耗模式\n", TAG);
}

void Compass::exitLowPowerMode()
{
    if (!_initialized) return;
    
    // 重新初始化QMC5883P
    if (!qmc.begin()) {
        Serial.printf("[%s] [警告] 退出低功耗模式时重新初始化失败\n", TAG);
        return;
    }
    delay(50); // 等待传感器稳定
    
    Serial.printf("[%s] 退出低功耗模式\n", TAG);
}

bool Compass::configureForDeepSleep()
{
    if (!_initialized) return false;
    
    // QMC5883L进入最低功耗模式
    enterLowPowerMode();
    
    Serial.printf("[%s] 已配置深度睡眠模式\n", TAG);
    return true;
}

#endif