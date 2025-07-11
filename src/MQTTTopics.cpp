#include "MQTTTopics.h"
#include "device.h"

// 定义全局的 MQTTTopics 实例
// 使用设备ID初始化，如果设备ID为空则使用默认值
MQTTTopics mqttTopics(device_state.device_id.isEmpty() ? "default_device" : device_state.device_id);
