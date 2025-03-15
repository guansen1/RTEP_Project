#ifndef I2C_HANDLE_H
#define I2C_HANDLE_H

#include "gpio/gpio.h"
#include "display/i2c_display.h"

class I2cDisplayHandle : public GPIO::GPIOEventCallbackInterface {
public:
    I2cDisplayHandle();
    virtual ~I2cDisplayHandle();

    // PIR 事件回调：上升沿切换为 INVASION
    virtual void handleEvent(const gpiod_line_event& event) override;

    // DHT 数据处理：当状态为 SAFE 时更新显示 SAFE 与温湿度数据
    void handleDHT(float temp, float humidity);

    // 键盘按键处理：在 INVASION 状态下，记录密码输入，若输入正确（"1234"）则解除 INVASION
    void handleKeyPress(char key);

private:
    enum class DisplayState { SAFE, INVASION };
    DisplayState state;
    std::string inputBuffer; // 新增：保存输入的密码
    float lastTemp;
    float lastHumidity;

};

#endif // I2C_HANDLE_H
