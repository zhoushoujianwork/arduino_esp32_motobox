#ifndef LBS_DATA_H
#define LBS_DATA_H

#include <Arduino.h>

// LBS请求状态
enum class LBSState {
    IDLE,           // 空闲状态
    REQUESTING,     // 正在请求
    WAIT_RESPONSE,  // 等待响应
    DONE,          // 完成
    ERROR          // 错误
};

struct lbs_data_t {
    bool valid;
    float latitude;
    float longitude;
    int radius;
    String descr;
    int stat;
    LBSState state;         // 当前状态
    unsigned long lastRequestTime;  // 上次请求时间
    unsigned long requestStartTime; // 请求开始时间
    String operator_name;
    unsigned long timestamp;
};

#endif // LBS_DATA_H 