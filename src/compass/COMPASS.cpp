#include "COMPASS.h"

COMPASS::COMPASS(int sda, int scl)
{
    this->sda = sda;
    this->scl = scl;
    
    // 初始化校准参数
    this->x_min = 32767;
    this->x_max = -32768;
    this->y_min = 32767;
    this->y_max = -32768;
    this->z_min = 32767;
    this->z_max = -32768;
    
    this->isCalibrated = false;
}

void COMPASS::begin()
{
    Serial.println("COMPASS init start");
    
    // 初始化I2C
    Wire.begin(sda, scl);
    
    delay(10);
    
    // 检查设备连接
    if (!checkConnection()) {
        Serial.println("QMC5883L compass not found!");
        return;
    }
    
    // 复位
    writeRegister(QMC5883L_REG_CONFIG_2, 0x80); // 软复位
    delay(10);
    
    // 配置QMC5883L
    // 设置为连续模式，输出率100Hz，范围2G，过采样率512
    uint8_t config = QMC5883L_MODE_CONTINUOUS | 
                     QMC5883L_ODR_100HZ | 
                     QMC5883L_RNG_2G | 
                     QMC5883L_OSR_512;
    
    writeRegister(QMC5883L_REG_CONFIG_1, config);
    
    // 设置复位周期寄存器
    writeRegister(QMC5883L_REG_PERIOD, 0x01);
    
    Serial.println("COMPASS init success");
}

void COMPASS::loop()
{
    int16_t x, y, z;
    
    // 读取原始数据
    if (!getRaw(x, y, z)) {
        return;
    }
    
    // 如果处于校准模式，更新最大最小值
    if (!isCalibrated) {
        if (x < x_min) x_min = x;
        if (x > x_max) x_max = x;
        if (y < y_min) y_min = y;
        if (y > y_max) y_max = y;
        if (z < z_min) z_min = z;
        if (z > z_max) z_max = z;
    }
    
    // 计算方位角并更新设备状态
    float heading = getHeading();
    
    // 获取IMU数据进行倾斜补偿
    if (device.get_device_state()->imuReady) {
        heading = getTiltCompensatedHeading(
            device.get_imu_data()->roll, 
            device.get_imu_data()->pitch
        );
    }
    
    // 更新设备状态中的heading值
    device.get_gps_data()->heading = heading;
}

bool COMPASS::getRaw(int16_t &x, int16_t &y, int16_t &z)
{
    // 检查数据是否准备好
    uint8_t status = readRegister(QMC5883L_REG_STATUS);
    if (!(status & 0x01)) {
        return false; // 数据未准备好
    }
    
    // 读取原始数据
    Wire.beginTransmission(QMC5883L_ADDRESS);
    Wire.write(QMC5883L_REG_X_LSB);
    Wire.endTransmission(false);
    
    Wire.requestFrom(QMC5883L_ADDRESS, (uint8_t)6, true);
    
    if (Wire.available() < 6) {
        return false;
    }
    
    // 读取X,Y,Z轴数据（低位在前）
    uint8_t xl = Wire.read();
    uint8_t xh = Wire.read();
    uint8_t yl = Wire.read();
    uint8_t yh = Wire.read();
    uint8_t zl = Wire.read();
    uint8_t zh = Wire.read();
    
    // 组合为16位有符号整数
    x = (int16_t)(xl | (xh << 8));
    y = (int16_t)(yl | (yh << 8));
    z = (int16_t)(zl | (zh << 8));
    
    // 应用硬铁校准（偏移量补偿）
    x -= hardIronOffset[0];
    y -= hardIronOffset[1];
    z -= hardIronOffset[2];
    
    // 应用软铁校准（矩阵变换）
    int16_t x_corrected = x * softIronMatrix[0][0] + y * softIronMatrix[0][1] + z * softIronMatrix[0][2];
    int16_t y_corrected = x * softIronMatrix[1][0] + y * softIronMatrix[1][1] + z * softIronMatrix[1][2];
    int16_t z_corrected = x * softIronMatrix[2][0] + y * softIronMatrix[2][1] + z * softIronMatrix[2][2];
    
    x = x_corrected;
    y = y_corrected;
    z = z_corrected;
    
    return true;
}

float COMPASS::getHeading()
{
    int16_t x, y, z;
    
    if (!getRaw(x, y, z)) {
        return 0.0;
    }
    
    // 计算方位角，注意Y/X与正北的关系
    float heading = atan2(y, x) * 180.0 / M_PI;
    
    // 调整到0-360度范围
    if (heading < 0) {
        heading += 360.0;
    }
    
    return heading;
}

float COMPASS::getTiltCompensatedHeading(float roll, float pitch)
{
    int16_t x, y, z;
    
    if (!getRaw(x, y, z)) {
        return 0.0;
    }
    
    // 转换为弧度
    float roll_rad = roll * M_PI / 180.0;
    float pitch_rad = pitch * M_PI / 180.0;
    
    // 倾斜补偿计算
    float x_h = x * cos(pitch_rad) + z * sin(pitch_rad);
    float y_h = x * sin(roll_rad) * sin(pitch_rad) + y * cos(roll_rad) - z * sin(roll_rad) * cos(pitch_rad);
    
    // 计算补偿后的方位角
    float heading = atan2(y_h, x_h) * 180.0 / M_PI;
    
    // 调整到0-360度范围
    if (heading < 0) {
        heading += 360.0;
    }
    
    return heading;
}

void COMPASS::calibrate()
{
    Serial.println("Starting compass calibration...");
    Serial.println("Please rotate the device in all directions for 20 seconds");
    
    // 重置校准数据
    x_min = 32767;
    x_max = -32768;
    y_min = 32767;
    y_max = -32768;
    z_min = 32767;
    z_max = -32768;
    
    // 校准过程标记为未完成
    isCalibrated = false;
    
    // 校准过程持续20秒
    unsigned long startTime = millis();
    while (millis() - startTime < 20000) {
        int16_t x, y, z;
        
        if (getRaw(x, y, z)) {
            // 更新最小最大值
            if (x < x_min) x_min = x;
            if (x > x_max) x_max = x;
            if (y < y_min) y_min = y;
            if (y > y_max) y_max = y;
            if (z < z_min) z_min = z;
            if (z > z_max) z_max = z;
        }
        
        delay(10);
    }
    
    // 计算偏移量（硬铁校准）
    hardIronOffset[0] = (x_min + x_max) / 2;
    hardIronOffset[1] = (y_min + y_max) / 2;
    hardIronOffset[2] = (z_min + z_max) / 2;
    
    // 计算比例因子（软铁校准）
    float x_range = (x_max - x_min) / 2.0;
    float y_range = (y_max - y_min) / 2.0;
    float z_range = (z_max - z_min) / 2.0;
    
    // 计算平均范围
    float avg_range = (x_range + y_range + z_range) / 3.0;
    
    // 设置软铁校准矩阵
    softIronMatrix[0][0] = avg_range / x_range;
    softIronMatrix[1][1] = avg_range / y_range;
    softIronMatrix[2][2] = avg_range / z_range;
    
    // 标记校准完成
    isCalibrated = true;
    
    Serial.println("Compass calibration completed!");
    Serial.print("Offsets: X=");
    Serial.print(hardIronOffset[0]);
    Serial.print(", Y=");
    Serial.print(hardIronOffset[1]);
    Serial.print(", Z=");
    Serial.println(hardIronOffset[2]);
}

float COMPASS::convertToMicroTesla(int16_t raw)
{
    // QMC5883L在±2G范围内，1G = 12000LSB
    // 1G = 100微特斯拉
    return raw * (100.0 / 12000.0);
}

void COMPASS::writeRegister(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(QMC5883L_ADDRESS);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

uint8_t COMPASS::readRegister(uint8_t reg)
{
    Wire.beginTransmission(QMC5883L_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission(false);
    
    Wire.requestFrom(QMC5883L_ADDRESS, (uint8_t)1, true);
    return Wire.read();
}

bool COMPASS::checkConnection()
{
    // 读取芯片ID并检查
    uint8_t chipId = readRegister(QMC5883L_REG_CHIP_ID);
    Serial.print("QMC5883L Chip ID: 0x");
    Serial.println(chipId, HEX);
    
    // QMC5883L一般返回0xFF作为芯片ID
    return (chipId == 0xFF);
}

// 在每个模式中单独创建全局实例
COMPASS compass(0, 0); // 先用0,0初始化，在main.cpp中创建真正的实例 