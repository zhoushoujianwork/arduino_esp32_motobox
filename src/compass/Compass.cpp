#include "compass/Compass.h"

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
        Serial.println("[罗盘] 数据无效");
        return;
    }
    
    Serial.printf("[罗盘] 航向: %.2f° (%.3f rad), 方向: %s (%s, %s), 磁场: X=%.2f Y=%.2f Z=%.2f\n", 
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
Compass::Compass(int sda, int scl) : _wire(Wire1) {
    _debug = true;
    _sda = sda;
    _scl = scl;
    _declination = -6.5f;  // 默认磁偏角，需要根据地理位置调整
    _initialized = false;
    _lastReadTime = 0;
    _lastDebugPrintTime = 0;
}

void Compass::setDebug(bool debug) {
    _debug = debug;
}

void Compass::debugPrint(const String& message) {
    if (_debug) {
        Serial.println("[罗盘] " + message);
    }
}

bool Compass::begin() {
    debugPrint(String("初始化: SDA=") + _sda + ", SCL=" + _scl + ", 磁偏角=" + _declination);
    
    // 先确保 Wire 是干净的
    _wire.end();
    delay(50);
    
    // 初始化I2C
    bool wireBeginSuccess = _wire.begin(_sda, _scl);
    if (!wireBeginSuccess) {
        debugPrint("Wire.begin() 失败!");
        return false;
    }
    debugPrint("Wire.begin() 成功");

    delay(100);  // 给一些初始化时间
    
    // 初始化QMC5883L
    qmc.init();
    qmc.setCalibrationOffsets(0, 0, 0);
    qmc.setCalibrationScales(1.0, 1.0, 1.0);
    
    _initialized = true;
    device_state.compassReady = true;
    
    debugPrint("初始化完成");
    return true;
}

bool Compass::update() {
    if (!_initialized) {
        return false;
    }

    qmc.read();
    int16_t x, y, z;
    getRawData(x, y, z);
    
    float heading = calculateHeading(x, y);
    updateCompassData(x, y, z, heading);
    
    return true;
}

void Compass::loop() {
    if (!_initialized) {
        return;
    }

    // 更新数据
    update();

    // 定期打印调试信息
    if (_debug && millis() - _lastDebugPrintTime > 2000) {
        _lastDebugPrintTime = millis();
        printCompassData();
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
        debugPrint("罗盘未初始化，无法校准");
        return false;
    }
    
    debugPrint("开始校准，请旋转模块...");
    qmc.calibrate();
    debugPrint("校准完成，请将以下参数写入代码：");
    
    Serial.printf("qmc.setCalibrationOffsets(%d, %d, %d);\n",
        qmc.getCalibrationOffset(0), qmc.getCalibrationOffset(1), qmc.getCalibrationOffset(2));
    Serial.printf("qmc.setCalibrationScales(%.2f, %.2f, %.2f);\n",
        qmc.getCalibrationScale(0), qmc.getCalibrationScale(1), qmc.getCalibrationScale(2));
    
    return true;
}

void Compass::setCalibration(int xOffset, int yOffset, int zOffset, float xScale, float yScale, float zScale) {
    qmc.setCalibrationOffsets(xOffset, yOffset, zOffset);
    qmc.setCalibrationScales(xScale, yScale, zScale);
    debugPrint("校准参数已设置");
}

void Compass::getRawData(int16_t &x, int16_t &y, int16_t &z) {
    x = qmc.getX();
    y = qmc.getY();
    z = qmc.getZ();
}

void Compass::setDeclination(float declination) {
    _declination = declination;
    debugPrint(String("磁偏角设置为: ") + declination + "°");
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
    device_state.compassReady = false;
    compass_data.isValid = false;
    debugPrint("罗盘已重置");
}

float Compass::calculateHeading(int16_t x, int16_t y) {
    float heading = atan2(y, x) * 180.0 / PI;
    heading += _declination;  // 应用磁偏角校正
    return normalizeHeading(heading);
}

void Compass::updateCompassData(int16_t x, int16_t y, int16_t z, float heading) {
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
}

