#include <iostream>
#include <thread>
#include <chrono>
#include "gpio/gpio.h"
#include "telegram.h"    // Telegram 消息发送接口

int main() {
    std::cout << "系统启动！" << std::endl;


    // 初始化 GPIO 模块
    GPIO gpio;
    gpio.gpio_init();

    // 注册原有的 GS 事件处理器（用于日志输出等）
    GSEventHandler gsHandler(gpio);
    gpio.registerCallback(GS_IO,&gsHandler);

    // 启动 GPIO 事件监听线程
    gpio.start();

    // 主循环保持运行
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    gpio.stop();
    std::cout << "退出程序。" << std::endl;
    return 0;
}
