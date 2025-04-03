#ifndef COMPASS_H
#define COMPASS_H

#include <Wire.h>
#include "device.h"
#include <QMC5883LCompass.h>


class Compass {
public:
    Compass(int sda, int scl);
    void begin();
    void loop();
    bool calibrate();  // 校准函数
    
    // 获取原始数据
    void getRawData(int16_t &x, int16_t &y, int16_t &z);
    
    // 获取处理后的航向角
    float getHeading(int16_t x, int16_t y, int16_t z);
    
    int getDirection(int16_t x, int16_t y, int16_t z);

    // 获取方向的char表示
    char* getDirectionChar(int direction);

    // 设置磁偏角校正值
    void setDeclination(float declination);

private:
    int _sda;
    int _scl;
    float _declination;  // 磁偏角校正值
    QMC5883LCompass qmc;  // QMC5883L传感器对象
    
    // 数据处理函数
    float calculateHeading(int16_t x, int16_t y);
};

extern Compass compass;  // 全局罗盘对象

#endif