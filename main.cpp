#include <iostream>
#include <thread>
#include <atomic>
#include "gpio/gpio.h"
#include "pir/pir.h"
#include "dht/dht.h"
#include "i2c_display.h"

// 新增的 I2C 显示事件处理器，继承自 GPIOEventCallbackInterface
class I2cDisplayHandler : public GPIO::GPIOEventCallbackInterface {
public:
    // 假设上升沿表示检测到运动（入侵），下降沿表示无运动（安全）
    void handleEvent(const gpiod_line_event& event) override {
        if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
            I2cDisplay::getInstance().displayIntrusion();
            std::cout << "屏幕显示触发报警\n";
        } else if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
            I2cDisplay::getInstance().displaySafe();
        }
    }
};

int main() {
    std::cout << "PIR 传感器监听系统启动！\n";

    // 初始化 I2C 显示模块
    I2cDisplay::getInstance().init();

    // 初始化 GPIO（红外检测、其他设备）
    GPIO gpio;
    gpio.gpio_init();  // 初始化 GPIO

    // 注册 PIR 事件处理器（你原来的处理器，负责打印信息等）
    PIREventHandler pirEventHandler(gpio);
    gpio.registerCallback(&pirEventHandler);

    // 注册 I2C 显示事件处理器，将根据 GPIO 事件更新屏幕显示
    I2cDisplayHandler displayHandler;
    gpio.registerCallback(&displayHandler);

    // 启动 GPIO 事件监听线程
    gpio.start();  
    
    // 初始化 DHT11 温湿度传感器，并注册回调显示温湿度信息
    DHT11 dht11(gpio);
    dht11.registerCallback([](const DHTReading& reading) {
        float fahrenheit = (reading.temp_celsius * 9 / 5) + 32;
        std::cout << "湿度: " << reading.humidity << "%, 温度: " 
                  << reading.temp_celsius << "C (" << fahrenheit << "F)\n";
    });
    dht11.start();

    // 主循环：事件回调由线程处理，这里仅作等待
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // 停止 GPIO 监听线程（实际不会执行到此处）
    gpio.stop();
    std::cout << "退出程序。\n";
    return 0;
}

