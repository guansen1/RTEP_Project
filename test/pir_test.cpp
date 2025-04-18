#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "gpio/gpio.h"
#include "pir/pir.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <string>

// Mock class for GPIO interface
class MockGPIO : public GPIO {
public:
    MOCK_METHOD(void, gpio_init, (), (override));
    MOCK_METHOD(bool, configGPIO, (int, int), (override));
    MOCK_METHOD(int, readGPIO, (int), (override));
    MOCK_METHOD(bool, writeGPIO, (int, int), (override));
    MOCK_METHOD(void, registerCallback, (int, GPIOEventCallbackInterface*), (override));
    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(void, stop, (), (override));
};

// Test fixture for PIR tests
class PIRTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test environment
    }

    void TearDown() override {
        // Clean up test environment
    }

    MockGPIO mockGpio;
};

// Test PIR initialization
TEST_F(PIRTest, Initialization) {
    // Create PIR event handler
    PIREventHandler pirHandler(mockGpio);
    
    // Verify constructor doesn't crash
    SUCCEED();
}

// Test PIR handling of rising edge events
TEST_F(PIRTest, HandleRisingEdgeEvent) {
    PIREventHandler pirHandler(mockGpio);
    
    gpiod_line_event event;
    event.event_type = GPIOD_LINE_EVENT_RISING_EDGE;
    
    testing::internal::CaptureStdout();
    pirHandler.handleEvent(event);
    
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_THAT(output, testing::HasSubstr("[PIR] Triggered"));
}

// Test PIR handling of falling edge events
TEST_F(PIRTest, HandleFallingEdgeEvent) {
    PIREventHandler pirHandler(mockGpio);
    
    gpiod_line_event event;
    event.event_type = GPIOD_LINE_EVENT_FALLING_EDGE;
    
    testing::internal::CaptureStdout();
    pirHandler.handleEvent(event);
    
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_THAT(output, testing::HasSubstr("[PIR] Trigger gone"));
}

// Test callback registration
TEST_F(PIRTest, RegisterCallback) {
    // Expect callback registration with correct parameters
    EXPECT_CALL(mockGpio, registerCallback(PIR_IO, testing::_))
        .Times(1);
    
    PIREventHandler pirHandler(mockGpio);
    mockGpio.registerCallback(PIR_IO, &pirHandler);
}

// Hardware integration test (enabled conditionally)
#ifdef HARDWARE_TEST_ENABLED
TEST(PIRHardwareTest, TestWithRealHardware) {
    GPIO gpio;
    gpio.gpio_init();
    
    PIREventHandler pirHandler(gpio);
    gpio.registerCallback(PIR_IO, &pirHandler);
    gpio.start();
    
    std::cout << "PIR hardware test: Please trigger sensor within 10 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    gpio.stop();
    std::cout << "PIR hardware test completed." << std::endl;
    
    SUCCEED();
}
#endif

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}