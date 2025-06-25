/*
 * ml307 模块总头文件
 */

#ifndef ml307_H
#define ml307_H

#include "Ml307AtModem.h"
#include "Ml307Mqtt.h"

// 全局实例已在各自的 cpp 文件中定义
extern Ml307AtModem ml307_at;
extern Ml307Mqtt Ml307Mqtt;

#endif // ml307_H 