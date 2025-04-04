#include "keyboard.h"
#include "display/i2c_handle.h"
using namespace std;

const int ActiveKeyboardScanner::rowPins[4] = {13, 16, 20, 21};
const int ActiveKeyboardScanner::colPins[4] = {1, 7, 8, 26};

// 4x4 keyboard key mapping table
const char ActiveKeyboardScanner::keyMap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

ActiveKeyboardScanner::ActiveKeyboardScanner(GPIO &gpioRef,I2cDisplayHandle &displayHandle)
    : gpio(gpioRef), scanning(false), timerfd(-1), displayHandle(displayHandle)
{

}

ActiveKeyboardScanner::~ActiveKeyboardScanner() {
    stop();
}

void ActiveKeyboardScanner::setKeyCallback(std::function<void(char)> callback) {
    keyCallback = callback;
}

void ActiveKeyboardScanner::start() {
    scanning = true;
    timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd == -1) {
        std::cerr << "Failed to create timerfd: " << strerror(errno) << std::endl;
        return;
    }
    // 2s timer
    struct itimerspec its;
    its.it_value.tv_sec = 0;  // first trigger time
    its.it_value.tv_nsec = 100000000;
    its.it_interval.tv_sec = 0;  // periodic trigger time
    its.it_interval.tv_nsec = 100000000;
    
    if (timerfd_settime(timerfd, 0, &its, NULL) == -1) {
        std::cerr << "Failed to set timerfd: " << strerror(errno) << std::endl;
        close(timerfd);
        timerfd = -1;
        return;
    }
    
    // Start worker thread
    scanThread = std::thread(&ActiveKeyboardScanner::scanLoop, this);
}

void ActiveKeyboardScanner::stop() {
    scanning = false;
    // Close time fd
    if (timerfd != -1) {
        close(timerfd);
        timerfd = -1;
    }
    if (scanThread.joinable()) {
        scanThread.join();
    }
}

void ActiveKeyboardScanner::closescan() {
    scanning = false;
}

void ActiveKeyboardScanner::timerEvent() {
    for (int col = 0; col < 4; col++) {
        // Set current column to low level
        gpio.writeGPIO(colPins[col], 0);
        // Wait for voltage stabilization (10 milliseconds)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        // Check all rows
        for (int row = 0; row < 4; row++) {
            int value = gpio.readGPIO(rowPins[row]);
            // When the key is pressed, the key connects the row to the low-level column, pulling down the row voltage
            if (value == 0) {
                char key = keyMap[row][col];
                if (keyCallback) {
                    keyCallback(key);
                } else {
                    std::cout << "[ActiveKeyboard] Key pressed: " << key << std::endl;
                }
                // Wait for the key to be released (simplified), avoid continuous detection
                while (gpio.readGPIO(rowPins[row]) == 0 && scanning) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                }
            }
        }
        // Restore current column to high level
        gpio.writeGPIO(colPins[col], 1);
        // Wait between columns
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void ActiveKeyboardScanner::scanLoop() {
    // Main scanning loop: drive each column to low level in turn, then read the state of all rows
    while (scanning) {
        if (timerfd == -1) {
            break;
        }
        
        // Block until trigger
        uint64_t exp;
        ssize_t s = read(timerfd, &exp, sizeof(uint64_t));
        if (s != sizeof(uint64_t)) {
            if (scanning) {
                std::cerr << "Failed to read timer: " << strerror(errno) << std::endl;
            }
            continue;
        }
        
        // Trigger, execute callback
        timerEvent();
    }
}

void ActiveKeyboardScanner::initkeyboard(I2cDisplayHandle& displayHandle) {
    setKeyCallback([&displayHandle](char key) {
        displayHandle.handleKeyPress(key);  // Call display handling logic
    });
}
