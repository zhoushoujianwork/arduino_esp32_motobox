#ifndef COMPASS_H
#define COMPASS_H

#include <Wire.h>
#include <QMC5883LCompass.h>
#include "config.h"
#include "device.h"

typedef struct
{
    float x;          // X轴磁场强度
    float y;          // Y轴磁场强度
    float z;          // Z轴磁场强度
    float heading; // 航向角 0-360
} compass_data_t;

extern compass_data_t compass_data;

void printCompassData();


/**
 * @brief QMC5883L 罗盘传感器驱动
 * 支持初始化、数据读取、方向获取、校准等功能
 */
class Compass {
public:
    /**
     * @brief 构造函数，指定I2C引脚
     * @param sda I2C SDA引脚
     * @param scl I2C SCL引脚
     */
    Compass(int sda, int scl);

    /**
     * @brief 读取罗盘数据并更新到 device
     */
    void loop();

    /**
     * @brief 启动校准流程（需手动调用，按提示旋转模块）
     * @return 是否成功
     */
    bool calibrate();

    /**
     * @brief 设置校准参数（建议校准后将参数写入代码）
     */
    void setCalibration(int xOffset, int yOffset, int zOffset, float xScale, float yScale, float zScale);

    /**
     * @brief 获取原始磁场数据
     */
    void getRawData(int16_t &x, int16_t &y, int16_t &z);

    /**
     * @brief 获取方向字符串（如"N", "NE"等）
     */
    const char* getDirectionChar(float heading);

    /**
     * @brief 设置磁偏角（单位：度）
     */
    void setDeclination(float declination);

    /**
     * @brief 获取磁偏角
     */
    float getDeclination();

    /**
     * @brief 初始化罗盘
     */
    void begin();

    /**
     * @brief 设置调试模式
     */
    void setDebug(bool debug);


private:
    int _sda;
    int _scl;
    bool _debug;
    bool _initialized;
    TwoWire& _wire; // 使用 Wire1 作为 I2C 总线
    float _declination;  // 磁偏角校正值
    QMC5883LCompass qmc;  // QMC5883L传感器对象
    
    // 数据处理函数
    float calculateHeading(int16_t x, int16_t y);

    void debugPrint(const String& message);
    unsigned long _lastDebugPrintTime;
};

#ifdef  ENABLE_COMPASS
extern Compass compass;  // 全局罗盘对象
#endif


#endif  