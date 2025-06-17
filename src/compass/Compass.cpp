#include "compass/Compass.h"

#ifdef ENABLE_COMPASS
Compass compass(GPS_COMPASS_SDA, GPS_COMPASS_SCL);

#endif

compass_data_t compass_data = {0, 0, 0, 0};

/**
 * @brief QMC5883L 罗盘传感器驱动实现
 */
Compass::Compass(int sda, int scl)
    :_wire(Wire)
{
    _debug = true;
    _sda = sda;
    _scl = scl;
    _declination = -6.5f;
    _initialized = false;
}

void Compass::setDebug(bool debug) {
    _debug = debug;
}

void Compass::debugPrint(const String& message) {
    if (_debug) {
        Serial.println(message);
    }
}

void Compass::begin() {
    Serial.printf("[罗盘] 初始化: SDA=%d, SCL=%d, 磁偏角=%.2f\n", _sda, _scl, _declination);
    
    // 先确保 Wire 是干净的
    _wire.end();
    delay(50);
    
    // 添加更多调试信息
    bool wireBeginSuccess = _wire.begin(_sda, _scl);
    if (!wireBeginSuccess) {
        Serial.println("[罗盘] Wire.begin() 失败!");
        return;
    }
    Serial.println("[罗盘] Wire.begin() 成功");

    device_state.compassReady = true;
    
    delay(100);  // 给一些初始化时间
    
    qmc.init();
    qmc.setCalibrationOffsets(0, 0, 0);
    qmc.setCalibrationScales(1.0, 1.0, 1.0);
    _initialized = true;
}

void Compass::loop() {
    if (!_initialized) {
        return;
    }

    qmc.read();
    int16_t x, y, z;
    getRawData(x, y, z);
    compass_data.x = x;
    compass_data.y = y;
    compass_data.z = z;
    compass_data.heading = calculateHeading(x, y);

    if (millis() - _lastDebugPrintTime > 1000) {
        _lastDebugPrintTime = millis();
        // printCompassData();
    }
}

bool Compass::calibrate() {
    Serial.println("[罗盘] 开始校准，请旋转模块...");
    qmc.calibrate();
    Serial.println("[罗盘] 校准完成，请将以下参数写入代码：");
    Serial.printf("qmc.setCalibrationOffsets(%d, %d, %d);\n",
        qmc.getCalibrationOffset(0), qmc.getCalibrationOffset(1), qmc.getCalibrationOffset(2));
    Serial.printf("qmc.setCalibrationScales(%.2f, %.2f, %.2f);\n",
        qmc.getCalibrationScale(0), qmc.getCalibrationScale(1), qmc.getCalibrationScale(2));
    return true;
}

void Compass::setCalibration(int xOffset, int yOffset, int zOffset, float xScale, float yScale, float zScale) {
    qmc.setCalibrationOffsets(xOffset, yOffset, zOffset);
    qmc.setCalibrationScales(xScale, yScale, zScale);
}

void Compass::getRawData(int16_t &x, int16_t &y, int16_t &z) {
    x = qmc.getX();
    y = qmc.getY();
    z = qmc.getZ();
}

void Compass::setDeclination(float declination) {
    _declination = declination;
}

float Compass::calculateHeading(int16_t x, int16_t y) {
    float heading = atan2(y, x) * 180.0 / PI;
    heading += _declination;
    if (heading < 0) heading += 360;
    if (heading >= 360) heading -= 360;
    return heading;
}


float Compass::getDeclination() {
    return _declination;
}


void printCompassData() {
    Serial.printf("Compass Heading: %f, X: %f, Y: %f, Z: %f, Direction: %s\n", 
        compass_data.heading, compass_data.x, compass_data.y, compass_data.z, getDirectionChar(compass_data.heading));
}

const char* getDirectionChar(float heading) {
    int direction = (int)((heading + 22.5) / 45.0);
    if (direction >= 8) direction = 0;
    static const char* directions[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
    return directions[direction];
}
