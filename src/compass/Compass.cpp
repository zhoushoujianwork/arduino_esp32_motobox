#include "compass/Compass.h"

Compass::Compass(int sda, int scl)  {
    Serial.println("[罗盘] 初始化...");
    _sda = sda;
    _scl = scl;
    _declination = -6.5f; // 江苏磁偏角
    Serial.printf("[罗盘] 磁偏角: %.2f度\n", _declination);
}

void Compass::begin() {
    Wire.begin(_sda, _scl);
    qmc.init();
    qmc.setMode(0x01, 0x0C, 0x10, 0x00);
}

void Compass::loop() {
    qmc.read();
    int16_t x, y, z;
    getRawData(x, y, z);
    compass_data_t compassData;
    compassData.x = x;
    compassData.y = y;
    compassData.z = z;
    compassData.direction = getDirection(x, y, z);
    device.set_compass_data(&compassData);
}

bool Compass::calibrate() {
    // 开始校准
    qmc.setCalibration(-4096, 4096, -4096, 4096, -4096, 4096);
    return true;
}

void Compass::getRawData(int16_t &x, int16_t &y, int16_t &z) {
    x = qmc.getX();
    y = qmc.getY();
    z = qmc.getZ();
}

float Compass::getHeading(int16_t x, int16_t y, int16_t z) {
    return calculateHeading(x, y);
}

void Compass::setDeclination(float declination) {
    _declination = declination;
}

float Compass::calculateHeading(int16_t x, int16_t y) {
    // 计算航向角（0-360度）
    float heading = atan2(y, x) * 180.0 / PI;
    
    // 应用磁偏角校正
    heading += _declination;
    
    // 确保角度在0-360度范围内
    if (heading < 0) {
        heading += 360;
    }
    if (heading >= 360) {
        heading -= 360;
    }
    
    return heading;
}

// 获取方向的数值表示 (0-7)
int Compass::getDirection(int16_t x, int16_t y, int16_t z) {
    float heading = getHeading(x, y, z);
    
    // 将360度分为8个方向，每个方向45度
    int direction = (int)((heading + 22.5) / 45.0);
    if (direction >= 8) {
        direction = 0;
    }

    return direction;
}

char* Compass::getDirectionChar(int direction) {
    static const char* directions[] = {
        "N", "NE", "E", "SE", "S", "SW", "W", "NW"
    };
    return (char*)directions[direction];
}


