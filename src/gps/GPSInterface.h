/*
 * GPS统一接口定义
 * 支持外部GPS模块和GSM模块GNSS功能
 */

#ifndef GPS_INTERFACE_H
#define GPS_INTERFACE_H

#include <Arduino.h>
#include "GPSData.h"

// 统一GPS接口类
class GPSInterface {
public:
    virtual ~GPSInterface() = default;
    virtual void begin() = 0;
    virtual void loop() = 0;
    virtual bool isReady() = 0;
    virtual gps_data_t getData() = 0;
    virtual void setDebug(bool debug) = 0;
    virtual String getType() const = 0; // 返回GPS类型标识
};

#endif // GPS_INTERFACE_H 