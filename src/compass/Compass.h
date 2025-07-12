#ifndef COMPASS_H
#define COMPASS_H

#include <Wire.h>
#include <QMC5883LCompass.h>
#include "config.h"
#include "device.h"

// 方向枚举
enum CompassDirection {
    NORTH = 0,      // 北
    NORTHEAST,      // 东北
    EAST,           // 东
    SOUTHEAST,      // 东南
    SOUTH,          // 南
    SOUTHWEST,      // 西南
    WEST,           // 西
    NORTHWEST       // 西北
};

// 罗盘数据结构
typedef struct {
    float x;                    // X轴磁场强度
    float y;                    // Y轴磁场强度
    float z;                    // Z轴磁场强度
    float heading;              // 航向角 0-360度
    float headingRadians;       // 航向角弧度
    CompassDirection direction; // 主要方向
    const char* directionStr;   // 方向字符串 (N, NE, E, SE, S, SW, W, NW)
    const char* directionName;  // 方向全名 (North, Northeast, etc.)
    const char* directionCN;    // 中文方向名
    bool isValid;               // 数据是否有效
    unsigned long timestamp;    // 数据时间戳
} compass_data_t;

extern compass_data_t compass_data;

// 工具函数
void printCompassData();
const char* getDirectionStr(float heading);
const char* getDirectionName(float heading);
const char* getDirectionCN(float heading);
CompassDirection getDirection(float heading);
float normalizeHeading(float heading);


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
     * @brief 初始化罗盘
     * @return 是否初始化成功
     */
    bool begin();

    /**
     * @brief 读取罗盘数据并更新到全局变量
     * @return 是否读取成功
     */
    bool update();

    /**
     * @brief 主循环函数
     */
    void loop();

    /**
     * @brief 获取当前航向角
     * @return 航向角 (0-360度)
     */
    float getHeading();

    /**
     * @brief 获取当前航向角弧度
     * @return 航向角弧度
     */
    float getHeadingRadians();

    /**
     * @brief 获取当前方向枚举
     * @return CompassDirection枚举值
     */
    CompassDirection getCurrentDirection();

    /**
     * @brief 获取当前方向字符串
     * @return 方向字符串 (N, NE, E, SE, S, SW, W, NW)
     */
    const char* getCurrentDirectionStr();

    /**
     * @brief 获取当前方向全名
     * @return 方向全名 (North, Northeast, etc.)
     */
    const char* getCurrentDirectionName();

    /**
     * @brief 获取当前方向中文名
     * @return 中文方向名
     */
    const char* getCurrentDirectionCN();

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
     * @brief 设置磁偏角（单位：度）
     */
    void setDeclination(float declination);

    /**
     * @brief 获取磁偏角
     */
    float getDeclination();

    /**
     * @brief 设置调试模式
     */
    void setDebug(bool debug);

    /**
     * @brief 检查罗盘是否已初始化
     */
    bool isInitialized();

    /**
     * @brief 检查数据是否有效
     */
    bool isDataValid();

    /**
     * @brief 重置罗盘
     */
    void reset();

private:
    int _sda;
    int _scl;
    bool _initialized;
    TwoWire& _wire;              // 使用 Wire1 作为 I2C 总线（与IMU共享）
    float _declination;          // 磁偏角校正值
    QMC5883LCompass qmc;         // QMC5883L传感器对象
    unsigned long _lastReadTime; // 上次读取时间
    unsigned long _lastDebugPrintTime;
    
    // 数据处理函数
    float calculateHeading(int16_t x, int16_t y);
    void updateCompassData(int16_t x, int16_t y, int16_t z, float heading);
};

#ifdef ENABLE_COMPASS
extern Compass compass;  // 全局罗盘对象
#endif

#endif