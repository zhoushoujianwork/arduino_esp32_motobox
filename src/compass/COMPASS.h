#ifndef COMPASS_H
#define COMPASS_H

#include <Wire.h>
#include <ArduinoJson.h>
#include "device.h"

// QMC5883L罗盘相关寄存器地址和值
#define QMC5883L_ADDRESS       0x0D  // QMC5883L的I2C地址

// 寄存器
#define QMC5883L_REG_X_LSB     0x00  // 输出数据寄存器X低位
#define QMC5883L_REG_X_MSB     0x01  // 输出数据寄存器X高位
#define QMC5883L_REG_Y_LSB     0x02  // 输出数据寄存器Y低位
#define QMC5883L_REG_Y_MSB     0x03  // 输出数据寄存器Y高位
#define QMC5883L_REG_Z_LSB     0x04  // 输出数据寄存器Z低位
#define QMC5883L_REG_Z_MSB     0x05  // 输出数据寄存器Z高位
#define QMC5883L_REG_STATUS    0x06  // 状态寄存器
#define QMC5883L_REG_TEMP_LSB  0x07  // 温度低位
#define QMC5883L_REG_TEMP_MSB  0x08  // 温度高位
#define QMC5883L_REG_CONFIG_1  0x09  // 配置寄存器1
#define QMC5883L_REG_CONFIG_2  0x0A  // 配置寄存器2
#define QMC5883L_REG_PERIOD    0x0B  // 设置/复位周期寄存器
#define QMC5883L_REG_CHIP_ID   0x0D  // 芯片ID (应为0xFF)

// 配置寄存器1位
#define QMC5883L_MODE_STANDBY  0b00000000  // 待机模式
#define QMC5883L_MODE_CONTINUOUS 0b00000001  // 连续测量模式

// 数据输出速率
#define QMC5883L_ODR_10HZ      0b00000000  // 10Hz
#define QMC5883L_ODR_50HZ      0b00000100  // 50Hz
#define QMC5883L_ODR_100HZ     0b00001000  // 100Hz
#define QMC5883L_ODR_200HZ     0b00001100  // 200Hz

// 量程范围
#define QMC5883L_RNG_2G        0b00000000  // ±2G
#define QMC5883L_RNG_8G        0b00010000  // ±8G

// 过采样率
#define QMC5883L_OSR_512       0b00000000  // 过采样率512
#define QMC5883L_OSR_256       0b01000000  // 过采样率256
#define QMC5883L_OSR_128       0b10000000  // 过采样率128
#define QMC5883L_OSR_64        0b11000000  // 过采样率64

class COMPASS
{
public:
    COMPASS(int sda, int scl);
    void begin();
    void loop();
    void calibrate();
    
    // 获取方位角（相对于北方的角度，0-359度）
    float getHeading();
    
    // 获取倾斜补偿后的方位角
    float getTiltCompensatedHeading(float roll, float pitch);
    
    // 原始数据读取
    bool getRaw(int16_t &x, int16_t &y, int16_t &z);
    
    // 将测量值转换为微特斯拉
    float convertToMicroTesla(int16_t raw);

private:
    int sda;
    int scl;
    
    int16_t x_min, x_max;
    int16_t y_min, y_max;
    int16_t z_min, z_max;
    
    bool isCalibrated;
    
    // 软铁校准矩阵
    float softIronMatrix[3][3] = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };
    
    // 硬铁偏移量
    int16_t hardIronOffset[3] = {0, 0, 0};
    
    // 写寄存器
    void writeRegister(uint8_t reg, uint8_t value);
    
    // 读寄存器
    uint8_t readRegister(uint8_t reg);
    
    // 检查设备连接
    bool checkConnection();
};

extern COMPASS compass;

#endif 