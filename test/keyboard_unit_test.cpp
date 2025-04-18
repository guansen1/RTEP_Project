#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "keyboard/keyboard.h"
#include "gpio/gpio.h"
#include <chrono>
#include <thread>
#include <atomic>

// Create a Mock GPIO class for unit testing
class MockGPIO : public GPIO {
public:
    std::unordered_map<int, int> pinValues;
    
    struct CallRecord {
        std::string method;
        int pin;
        int value;
    };
    std::vector<CallRecord> callHistory;
    
private:
    int pressedRow = -1;
    int pressedCol = -1;
    int currentScanningCol = -1;
public:
    
    // Constructor
    MockGPIO() {
        // Initialize all pins to high level (not pressed state)
        const int rowPins[4] = {13, 16, 20, 21};
        const int colPins[4] = {1, 7, 8, 26};
        
        for (int i = 0; i < 4; i++) {
            pinValues[rowPins[i]] = 1;
            pinValues[colPins[i]] = 1;
        }
    }
    
    ~MockGPIO() override {}
    
    void gpio_init() override {
        callHistory.push_back({"gpio_init", 0, 0});
    }
    
    // Override GPIO methods to make them fully controllable
    bool configGPIO(int pin_number, int config_num) override {
        callHistory.push_back({"configGPIO", pin_number, config_num});
        return true;
    }
    
    int readGPIO(int pin_number) override {
    callHistory.push_back({"readGPIO", pin_number, 0});
    
    // If it's a row pin
    const int rowPins[4] = {13, 16, 20, 21};
    for (int i = 0; i < 4; i++) {
        if (pin_number == rowPins[i]) {
            if (currentScanningCol == pressedCol && i == pressedRow) {
                return 0; // Return pressed state (low level)
            }
        }
    }
    
    // Return high level (not pressed) in other cases
    return pinValues.count(pin_number) ? pinValues[pin_number] : 1;
}
    
    bool writeGPIO(int pin_number, int value) override {
    callHistory.push_back({"writeGPIO", pin_number, value});
    pinValues[pin_number] = value;
    
    // Check if setting a column pin
    const int colPins[4] = {1, 7, 8, 26};
    for (int i = 0; i < 4; i++) {
        if (pin_number == colPins[i]) {
            if (value == 0) {
                currentScanningCol = i; // Record the current scanning column
            }
            break;
        }
    }
    
    return true;
}
    
    void registerCallback(int pin_number, GPIOEventCallbackInterface* callback) override {
        callHistory.push_back({"registerCallback", pin_number, 0});
    }
    
    void start() override {
        callHistory.push_back({"start", 0, 0});
    }
    
    void stop() override {
        callHistory.push_back({"stop", 0, 0});
    }
    
    // Helper testing methods
    void clearHistory() {
        callHistory.clear();
    }
    
    // Set pin state to simulate key presses
    void setPinValue(int pin, int value) {
        pinValues[pin] = value;
    }
    
    // Simulate pressing a key at specific row and column
    void simulateKeyPress(int row, int col) {
    pressedRow = row;
    pressedCol = col;
}
    
    // Simulate releasing all keys
    void simulateAllKeysReleased() {
        const int rowPins[4] = {13, 16, 20, 21};
        const int colPins[4] = {1, 7, 8, 26};
        
        for (int i = 0; i < 4; i++) {
            pinValues[rowPins[i]] = 1;
            pinValues[colPins[i]] = 1;
        }
        pressedRow = -1;
    pressedCol = -1;
    }
};

// Unit test fixture class
class KeyboardUnitTest : public ::testing::Test {
protected:
    MockGPIO mockGpio;
    std::unique_ptr<ActiveKeyboardScanner> scanner;
    std::vector<char> detectedKeys;
    std::atomic<bool> callbackCalled;
    
    void SetUp() override {
        scanner = std::make_unique<ActiveKeyboardScanner>(mockGpio);
        detectedKeys.clear();
        callbackCalled = false;
        
        // Set keyboard callback function
        scanner->setKeyCallback([this](char key) {
            detectedKeys.push_back(key);
            callbackCalled = true;
        });
    }
    
    void TearDown() override {
        if (scanner) {
            scanner->stop();
            scanner.reset();
        }
    }
    
    // Wait for callback to be called or timeout
    bool waitForCallback(int timeoutMs = 500) {
        auto start = std::chrono::steady_clock::now();
        while (!callbackCalled) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            if (elapsed > timeoutMs) {
                return false;
            }
        }
        return true;
    }
};

// Test keyboard scanner initialization
TEST_F(KeyboardUnitTest, Initialization) {
    ASSERT_NO_THROW({
        ActiveKeyboardScanner testScanner(mockGpio);
    });
}

// Test start and stop scanning
TEST_F(KeyboardUnitTest, StartAndStop) {
    mockGpio.clearHistory();
    
    scanner->start();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    scanner->stop();
    
    bool writeGpioCalled = false;
    for (const auto& call : mockGpio.callHistory) {
        if (call.method == "writeGPIO") {
            writeGpioCalled = true;
            break;
        }
    }
    EXPECT_TRUE(writeGpioCalled);
}

// Test key detection
TEST_F(KeyboardUnitTest, KeyDetection) {
    scanner->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    mockGpio.simulateKeyPress(0, 0);
    
    EXPECT_TRUE(waitForCallback(1000));
    
    ASSERT_FALSE(detectedKeys.empty());
    EXPECT_EQ(detectedKeys[0], '1');
    
    callbackCalled = false;
    detectedKeys.clear();
    mockGpio.simulateAllKeysReleased();
    
    scanner->stop();
}

// Test multiple key sequence
TEST_F(KeyboardUnitTest, KeySequence) {
    scanner->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    mockGpio.simulateKeyPress(0, 0);
    
    EXPECT_TRUE(waitForCallback(1000));
    
    ASSERT_FALSE(detectedKeys.empty());
    EXPECT_EQ(detectedKeys[0], '1');
    
    callbackCalled = false;
    mockGpio.simulateAllKeysReleased();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    mockGpio.simulateKeyPress(1, 1);
    detectedKeys.clear();
    callbackCalled = false;
    
    EXPECT_TRUE(waitForCallback(1000));
    
    ASSERT_FALSE(detectedKeys.empty());
    EXPECT_EQ(detectedKeys[0], '5');
    
    mockGpio.simulateAllKeysReleased();
    
    scanner->stop();
}

// Test callback function
TEST_F(KeyboardUnitTest, CallbackFunction) {
    bool testCallbackCalled = false;
    char detectedKey = '\0';
    
    scanner->setKeyCallback([&testCallbackCalled, &detectedKey](char key) {
        testCallbackCalled = true;
        detectedKey = key;
    });
    
    scanner->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    mockGpio.simulateKeyPress(2, 2);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_TRUE(testCallbackCalled);
    EXPECT_EQ(detectedKey, '9');
    
    scanner->stop();
}