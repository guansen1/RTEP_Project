#ifndef GPIO_H
#define GPIO_H

#include <gpiod.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>

enum GPIOconfig{
    INPUT = 0,              // 输入模式
    OUTPUT = 1,             // 输出模式
    INPUT_PULLUP = 2,       // 上拉输入
    INPUT_PULLDOWN = 3,     // 下拉输入
    RISING_EDGE = 4,        // 上升沿触发事件
    FALLING_EDGE = 5,       // 下降沿触发事件
    BOTH_EDGES = 6          // 双边沿触发事件
};
enum GPIOdef{
    KB_R1_IO = 1,
    KB_R2_IO = 7,
    KB_R3_IO = 8,
    KB_R4_IO = 11,
    KB_R5_IO = 12,
    PIR_IO = 14,
    BUZZER_IO = 15,
    KB_R6_IO = 16,
    DHT_IO = 18,
    KB_R7_IO = 20,
    KB_R8_IO = 21
    
};

class GPIO {
    public:
        struct GPIOEventCallbackInterface {
            virtual void handleEvent(const gpiod_line_event& event) = 0;
            virtual ~GPIOEventCallbackInterface() = default;
        };
    
        GPIO();
        ~GPIO();
    
        void gpio_init();
        bool configGPIO(int pin_number, int config_num);
        int readGPIO(int pin_number);
        bool writeGPIO(int pin_number, int value);
        void registerCallback(int pin_number, GPIOEventCallbackInterface* callback);
        void start();
        void stop();
    
    private:
        void worker();
    
        int waitForEvent(int pin_number, struct timespec* timeout);
        bool readEvent(int pin_number, gpiod_line_event& event);
    
        struct gpiod_chip* chip;
        std::unordered_map<int, struct gpiod_line*> gpio_pins;
        std::unordered_map<int, int> gpio_config;
        std::unordered_map<int, std::vector<GPIOEventCallbackInterface*>> callbacks;
        std::thread workerThread;
        std::atomic<bool> running;
    };
    
#endif // GPIO_H
