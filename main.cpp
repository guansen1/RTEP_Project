// main.cpp
#include <iostream>
#include <thread>
#include <atomic>
#include "gpio/gpio.h"
#include "pir/pir.h"
#include "dht/dht.h"

int main() {
    std::cout << "PIR 传感器监听系统启动！\n";

    GPIO gpio;
    gpio.gpio_init();  // 初始化 GPIO

    PIREventHandler pirEventHandler(gpio);
    gpio.registerCallback(&pirEventHandler);
    gpio.start();  
    
    DHT11 dht11(gpio);
    dht11.registerCallback([](const DHTReading& reading) {
        float fahrenheit = (reading.temp_celsius * 9 / 5) + 32;
        std::cout << "湿度: " << reading.humidity << "%, 温度: " << reading.temp_celsius << "C (" << fahrenheit << "F)\n";
    });
    dht11.start();

    // 主程序进入事件循环
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    gpio.stop();  // 停止 GPIO 事件监听线程
    std::cout << "退出程序。\n";
    return 0;
}