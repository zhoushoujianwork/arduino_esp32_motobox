/*
 * ML370 模块总头文件
 */

#ifndef ML370_H
#define ML370_H

#include "Ml370AtModem.h"
#include "Ml370Mqtt.h"

// 全局实例已在各自的 cpp 文件中定义
extern Ml370AtModem ml370;
extern Ml370Mqtt ml370Mqtt;

#endif // ML370_H 