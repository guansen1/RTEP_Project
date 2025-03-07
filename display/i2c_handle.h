#ifndef I2C_HANDLE_H
#define I2C_HANDLE_H

#include "gpio/gpio.h"
#include "display/i2c_display.h"

// 类名为 I2cDisplayHandle
class I2cDisplayHandle : public GPIO::GPIOEventCallbackInterface {
public:
    I2cDisplayHandle();
    virtual ~I2cDisplayHandle();

    // PIR 事件回调：上升沿切换为 INVASION，下降沿切换为 SAFE
    virtual void handleEvent(const gpiod_line_event& event) override;

    // DHT 数据处理：当状态为 SAFE 时更新显示 SAFE 和温湿度数据
    void handleDHT(float temp, float humidity);

private:
    enum class DisplayState { SAFE, INVASION };
    DisplayState state;
};

#endif // I2C_HANDLE_H
