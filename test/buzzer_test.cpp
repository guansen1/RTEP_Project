#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "buzzer.h"

// Create Mock class for RPI_PWM
class MockRPI_PWM : public RPI_PWM {
public:
    // Override original methods so we can monitor calls
    int start(int channel, int frequency, float duty_cycle = 0, int chip = 0) override {
        return MockStart(channel, frequency, duty_cycle, chip);
    }
    
    void stop() override {
        MockStop();
    }
    
    int setDutyCycle(float v) const override {
        return MockSetDutyCycle(v);
    }
    
    // Create mock methods that can be monitored
    MOCK_METHOD(int, MockStart, (int channel, int frequency, float duty_cycle, int chip));
    MOCK_METHOD(void, MockStop, ());
    MOCK_METHOD(int, MockSetDutyCycle, (float v), (const));
};

// Test fixture
class BuzzerTest : public ::testing::Test {
protected:
    void SetUp() override {
        
    }

    void TearDown() override {
        
    }

    MockRPI_PWM mockPwm;
};

// Test if enable method correctly calls PWM
TEST_F(BuzzerTest, EnableCallsPWMWithCorrectParameters) {
    // Arrange: set up test object and expectations
    Buzzer buzzer(mockPwm);
    EXPECT_CALL(mockPwm, MockStart(BUZZER_PWM_CHANNEL, 1000, ::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(1));
    EXPECT_CALL(mockPwm, MockSetDutyCycle(50))
        .Times(1)
        .WillOnce(::testing::Return(1));
    
    // Act: execute the function being tested
    buzzer.enable(1000);
}

// Test different frequency values
TEST_F(BuzzerTest, EnableSupportsVariousFrequencies) {
    Buzzer buzzer(mockPwm);
    
    // Test different frequency values
    int testFrequencies[] = {100, 500, 1000, 2000, 4000};
    
    for (int freq : testFrequencies) {
        EXPECT_CALL(mockPwm, MockStart(BUZZER_PWM_CHANNEL, freq, ::testing::_, ::testing::_))
            .Times(1)
            .WillOnce(::testing::Return(1));
        EXPECT_CALL(mockPwm, MockSetDutyCycle(50))
            .Times(1)
            .WillOnce(::testing::Return(1));
        
        buzzer.enable(freq);
    }
}

// Test if disable method correctly calls PWM
TEST_F(BuzzerTest, DisableCallsStopMethod) {
    Buzzer buzzer(mockPwm);
    EXPECT_CALL(mockPwm, MockStop())
        .Times(2);
    
    buzzer.disable();
}

// Test if destructor calls disable
TEST_F(BuzzerTest, DestructorCallsDisable) {
    EXPECT_CALL(mockPwm, MockStop())
        .Times(1);
    
    {
        Buzzer buzzer(mockPwm);
        // Destructor should be called when leaving scope
    }
}

// Test handling of PWM call failures
TEST_F(BuzzerTest, HandlesFailureInPWMStart) {
    Buzzer buzzer(mockPwm);
    
    EXPECT_CALL(mockPwm, MockStart(BUZZER_PWM_CHANNEL, 1000, ::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(-1)); // Return failure
    
    buzzer.enable(1000);
}

// For hardware testing if needed
#ifdef HARDWARE_TEST_ENABLED
TEST(BuzzerHardwareTest, BuzzerPlaysSound) {
    // Create actual PWM controller
    RPI_PWM realPwm;
    Buzzer buzzer(realPwm);
    
    // Play different frequencies
    buzzer.enable(262); // C4 note
    sleep(1);
    
    buzzer.enable(330); // E4 note
    sleep(1);
    
    buzzer.enable(392); // G4 note
    sleep(1);
    
    buzzer.disable();
    
    // This test has no assertions, it's a manual verification test
    SUCCEED();
}
#endif