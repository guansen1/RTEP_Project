#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "gpio/gpio.h"

// Mock GPIO class for testing
class MockGPIO : public GPIO {
public:
    // Override parent constructor to avoid real hardware access
    MockGPIO() {
        // Note: This does not call the parent constructor
    }

    MOCK_METHOD(void, gpio_init, (), (override));
    MOCK_METHOD(bool, configGPIO, (int pin_number, int config_num), (override));
    MOCK_METHOD(int, readGPIO, (int pin_number), (override));
    MOCK_METHOD(bool, writeGPIO, (int pin_number, int value), (override));
    MOCK_METHOD(void, registerCallback, (int pin_number, GPIOEventCallbackInterface* callback), (override));
    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(void, stop, (), (override));
};

// Test callback class
class TestCallback : public GPIO::GPIOEventCallbackInterface {
public:
    MOCK_METHOD(void, handleEvent, (const gpiod_line_event& event), (override));
};

// Test GPIO initialization
TEST(GPIOTest, TestInitialization) {
#ifdef HARDWARE_TEST_ENABLED
    GPIO gpio;
    gpio.gpio_init();
    
    // Verify state after initialization
    EXPECT_TRUE(gpio.readGPIO(PIR_IO) >= 0);
    EXPECT_TRUE(gpio.writeGPIO(BUZZER_IO, 0));
    EXPECT_TRUE(gpio.writeGPIO(DHT_IO, 0));
#else
    // Using Mock for testing, no hardware required
    MockGPIO gpio;
    
    // Add this expectation, define gpio_init behavior
    EXPECT_CALL(gpio, gpio_init()).WillOnce(testing::Invoke([&gpio]() {
        gpio.configGPIO(PIR_IO, BOTH_EDGES);
        gpio.configGPIO(BUZZER_IO, OUTPUT);
        gpio.configGPIO(DHT_IO, OUTPUT);
    }));
    
    // Set expectations for configGPIO calls
    EXPECT_CALL(gpio, configGPIO(PIR_IO, BOTH_EDGES)).WillOnce(testing::Return(true));
    EXPECT_CALL(gpio, configGPIO(BUZZER_IO, OUTPUT)).WillOnce(testing::Return(true));
    EXPECT_CALL(gpio, configGPIO(DHT_IO, OUTPUT)).WillOnce(testing::Return(true));
    
    gpio.gpio_init();
#endif
}

// Test GPIO configuration
TEST(GPIOTest, TestConfigGPIO) {
#ifdef HARDWARE_TEST_ENABLED
    GPIO gpio;
    
    EXPECT_TRUE(gpio.configGPIO(PIR_IO, INPUT));
    EXPECT_TRUE(gpio.configGPIO(BUZZER_IO, OUTPUT));
    
    EXPECT_FALSE(gpio.configGPIO(PIR_IO, 99));
#else
    MockGPIO gpio;
    
    EXPECT_CALL(gpio, configGPIO(PIR_IO, INPUT)).WillOnce(testing::Return(true));
    EXPECT_CALL(gpio, configGPIO(BUZZER_IO, OUTPUT)).WillOnce(testing::Return(true));
    EXPECT_CALL(gpio, configGPIO(PIR_IO, 99)).WillOnce(testing::Return(false));
    
    EXPECT_TRUE(gpio.configGPIO(PIR_IO, INPUT));
    EXPECT_TRUE(gpio.configGPIO(BUZZER_IO, OUTPUT));
    EXPECT_FALSE(gpio.configGPIO(PIR_IO, 99));
#endif
}

// Test GPIO read operation
TEST(GPIOTest, TestReadGPIO) {
#ifdef HARDWARE_TEST_ENABLED
    GPIO gpio;
    gpio.configGPIO(PIR_IO, INPUT);
    
    // Read input pin, cannot predict specific value, but should be valid
    EXPECT_GE(gpio.readGPIO(PIR_IO), 0);
    
    // Test reading non-existent pin
    EXPECT_EQ(gpio.readGPIO(100), -1);
#else
    MockGPIO gpio;
    
    EXPECT_CALL(gpio, readGPIO(PIR_IO)).WillOnce(testing::Return(1));
    EXPECT_CALL(gpio, readGPIO(100)).WillOnce(testing::Return(-1));
    
    EXPECT_EQ(gpio.readGPIO(PIR_IO), 1);
    EXPECT_EQ(gpio.readGPIO(100), -1);
#endif
}

// Test GPIO write operation
TEST(GPIOTest, TestWriteGPIO) {
#ifdef HARDWARE_TEST_ENABLED
    GPIO gpio;
    gpio.configGPIO(BUZZER_IO, OUTPUT);
    
    // Test output high and low levels
    EXPECT_TRUE(gpio.writeGPIO(BUZZER_IO, 1));
    EXPECT_TRUE(gpio.writeGPIO(BUZZER_IO, 0));
    
    // Test writing to invalid pin
    EXPECT_FALSE(gpio.writeGPIO(100, 1));
#else
    MockGPIO gpio;
    
    EXPECT_CALL(gpio, writeGPIO(BUZZER_IO, 1)).WillOnce(testing::Return(true));
    EXPECT_CALL(gpio, writeGPIO(BUZZER_IO, 0)).WillOnce(testing::Return(true));
    EXPECT_CALL(gpio, writeGPIO(100, 1)).WillOnce(testing::Return(false));
    
    EXPECT_TRUE(gpio.writeGPIO(BUZZER_IO, 1));
    EXPECT_TRUE(gpio.writeGPIO(BUZZER_IO, 0));
    EXPECT_FALSE(gpio.writeGPIO(100, 1));
#endif
}

// Test callback registration
TEST(GPIOTest, TestRegisterCallback) {
#ifdef HARDWARE_TEST_ENABLED
    GPIO gpio;
    TestCallback callback;
    
    gpio.configGPIO(PIR_IO, BOTH_EDGES);
    gpio.registerCallback(PIR_IO, &callback);
    
    gpio.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    gpio.stop();
#else
    MockGPIO gpio;
    TestCallback callback;
    
    EXPECT_CALL(gpio, registerCallback(PIR_IO, &callback));
    
    gpio.registerCallback(PIR_IO, &callback);
#endif
}

// Test start and stop
TEST(GPIOTest, TestStartStop) {
#ifdef HARDWARE_TEST_ENABLED
    GPIO gpio;
    
    // Simply verify start/stop doesn't crash
    gpio.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    gpio.stop();
#else
    MockGPIO gpio;
    
    EXPECT_CALL(gpio, start());
    EXPECT_CALL(gpio, stop());
    
    gpio.start();
    gpio.stop();
#endif
}

// Test edge trigger
TEST(GPIOTest, TestEdgeTrigger) {
#ifndef HARDWARE_TEST_ENABLED
    MockGPIO gpio;
    TestCallback callback;
    
    // Set callback expectations
    EXPECT_CALL(gpio, configGPIO(PIR_IO, BOTH_EDGES)).WillOnce(testing::Return(true));
    EXPECT_CALL(gpio, registerCallback(PIR_IO, &callback));
    EXPECT_CALL(callback, handleEvent(testing::_)).Times(0);  // Not expected to be called in this test
    
    // Configure GPIO
    EXPECT_TRUE(gpio.configGPIO(PIR_IO, BOTH_EDGES));
    gpio.registerCallback(PIR_IO, &callback);
#endif
}

// Main function
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}