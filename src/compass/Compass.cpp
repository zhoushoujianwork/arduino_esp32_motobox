#include "compass/Compass.h"

Compass::Compass(int sda, int scl) : _sda(sda), _scl(scl), _declination(0.0f) {
}

void Compass::begin() {
    Serial.println("开始初始化磁力计 sda:" + String(_sda) + " scl:" + String(_scl));
    Wire.begin(_sda, _scl);
    qmc.init();
    qmc.setMode(0x01, 0x0C, 0x10, 0x00); // 设置连续测量模式，8Hz采样率，2G量程
    Serial.println("开始初始化磁力计");
}

void Compass::loop() {
    qmc.read();
    updateDeviceState();
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

float Compass::getHeading() {
    int16_t x, y, z;
    getRawData(x, y, z);
    return calculateHeading(x, y);
}

void Compass::setDeclination(float declination) {
    _declination = declination;
}

float Compass::calculateHeading(int16_t x, int16_t y) {
    float heading = atan2(y, x) * 180.0 / PI;
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

void Compass::updateDeviceState() {
    // 更新设备状态，可以根据需要添加更多功能
    float heading = getHeading();
    // TODO: 更新设备状态
}
