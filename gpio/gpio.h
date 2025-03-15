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
    INPUT = 0,              // input
    OUTPUT = 1,             // output
    INPUT_PULLUP = 2,       // pull up
    INPUT_PULLDOWN = 3,     // pull down
    RISING_EDGE = 4,        // RISING_EDGE
    FALLING_EDGE = 5,       // FALLING_EDGE
    BOTH_EDGES = 6          // BOTH_EDGES
};
enum GPIOdef{
    PIR_IO = 14,
    BUZZER_IO = 15,
    DHT_IO = 18
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