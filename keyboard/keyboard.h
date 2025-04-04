#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "gpio/gpio.h"
#include <string.h>
#include <functional>
#include <thread>
#include <atomic>
#include <iostream>
#include <chrono>
#include <unistd.h>
#include <sys/timerfd.h>
#include "display/i2c_display.h"

class I2cDisplayHandle;
class ActiveKeyboardScanner {
public:
    // Constructor receives a GPIO object reference
    ActiveKeyboardScanner(GPIO &gpio,I2cDisplayHandle &displayHandle);
    ~ActiveKeyboardScanner();

    // Start active scanning
    void start();
    // Stop scanning
    void stop();
    void closescan();
    // Set the key callback function, which will be called when a key is detected and pass the corresponding character
    void setKeyCallback(std::function<void(char)> callback);
    void initkeyboard(I2cDisplayHandle& displayHandle);

private:
    // Main scanning loop
    void scanLoop();
    void timerEvent();  // New: Timer event handler
    GPIO &gpio;
    std::thread scanThread;
    std::atomic<bool> scanning;
    int timerfd;  // New: Timer file descriptor
    std::function<void(char)> keyCallback;
    I2cDisplayHandle &displayHandle;
    // Keyboard row and column pins, modify according to actual hardware wiring:
    static const int rowPins[4];  // Example: connected to GPIO 5,6,7,8
    static const int colPins[4];  // Example: connected to GPIO 1,2,3,4

    // Keyboard key mapping table, row and column correspondences
    static const char keyMap[4][4];
};

#endif // KEYBOARD_H
