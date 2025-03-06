#ifndef MQ2SENSOR_H
#define MQ2SENSOR_H

#include "ADS1115.h"

class MQ2Sensor {
public:
    // 构造函数，传入 ADS1115 对象指针
    MQ2Sensor(ADS1115* adc);
    
    // 获取传感器数据（可在此处加入数据校准或转换）
    int16_t getSensorReading();

private:
    ADS1115* adc;
};

#endif // MQ2SENSOR_H
