#ifndef I2C_HANDLE_H
#define I2C_HANDLE_H

#include "gpio/gpio.h"
#include "display/i2c_display.h"
#include "buzzer/buzzer.h"  // 包含 buzzer 模块
#include "keyboard/keyboard.h"
class I2cDisplayHandle : public GPIO::GPIOEventCallbackInterface {
public:
    // 构造函数接收一个 Buzzer 引用
    I2cDisplayHandle(Buzzer &buzzerRef);
    virtual ~I2cDisplayHandle();

    // PIR 事件回调：上升沿切换为 INVASION（下降沿不做处理）
    virtual void handleEvent(const gpiod_line_event &event) override;

    // DHT 数据处理：更新最新数据，并在 SAFE 状态下刷新显示
    void handleDHT(float temp, float humidity);

    // 键盘按键处理：在 INVASION 状态下记录密码输入，验证后解除 INVASION
    void handleKeyPress(char key);

private:
    enum class DisplayState { SAFE, INVASION };
    DisplayState state;
    std::string inputBuffer; // 保存密码输入
    float lastTemp;
    float lastHumidity;
    Buzzer &buzzer;  // 通过构造函数注入，不使用全局变量
};

#endif // I2C_HANDLE_H
