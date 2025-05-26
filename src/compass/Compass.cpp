#include "compass/Compass.h"

Compass::Compass(int sda, int scl)  {
    Serial.println("[罗盘] 初始化...");
    _sda = sda;
    _scl = scl;
    _declination = -6.5f; // 江苏磁偏角
    Serial.println("[罗盘] sda: " + String(_sda) + " scl: " + String(_scl));
    Serial.printf("[罗盘] 磁偏角: %.2f度\n", _declination);
}

// 库使用的Wire 这里必须用这个
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
    compassData.heading = calculateHeading(x, y);
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

char* Compass::getDirectionChar(float heading) {
     // 将360度分为8个方向，每个方向45度
    int direction = (int)((heading + 22.5) / 45.0);
    if (direction >= 8) {
        direction = 0;
    }
    static const char* directions[] = {
        "N", "NE", "E", "SE", "S", "SW", "W", "NW"
    };
    return (char*)directions[direction];
}


