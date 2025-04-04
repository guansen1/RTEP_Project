#include "i2c_handle.h"
#include <iostream>
#include <thread>
#include <string>

I2cDisplayHandle::I2cDisplayHandle(Buzzer &buzzerRefr)
    : state(DisplayState::SAFE), inputBuffer(""), lastTemp(0.0f), lastHumidity(0.0f), buzzer(buzzerRefr){
}

I2cDisplayHandle::~I2cDisplayHandle() {
}

void I2cDisplayHandle::handleEvent(const gpiod_line_event &event) {
    if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
        if (state == DisplayState ::SAFE){
        state = DisplayState::INVASION;
        inputBuffer.clear();  // Clear password input
        I2cDisplay::getInstance().displayIntrusion();
        std::cout << "[I2cDisplayHandle] PIR rising: state switched to INVASION" << std::endl;
        // Optional: Start alarm when entering invasion state
        //buzzer.startAlarm();
    }
    }
    // No processing for PIR falling edge, maintain INVASION state until keypad unlocks
}

void I2cDisplayHandle::handleDHT(float temp, float humidity) {
    // Update latest temperature and humidity data
    lastTemp = temp;
    lastHumidity = humidity;
    if (state == DisplayState::SAFE) {
        std::string tempStr = "Temp:" + std::to_string(temp) + " C";
        std::string humStr  = "Hum:"  + std::to_string(humidity) + " %";
        I2cDisplay::getInstance().displaySafeAndDHT(tempStr, humStr);
        std::cout << "[I2cDisplayHandle] DHT update: Temp " << temp 
                  << " C, Hum " << humidity << "%" << std::endl;
    }
}

void I2cDisplayHandle::handleKeyPress(char key) {
    if (state == DisplayState::INVASION) {
        // Process only numeric key input
        //if (key >= '0' && key <= '9') {
            inputBuffer.push_back(key);
            std::cout << "[I2cDisplayHandle] Password input: " << inputBuffer << std::endl;
            // Verify password when 4 digits are entered
            if (inputBuffer.size() == 4) {
                if (inputBuffer == "1234") {
                    state = DisplayState::SAFE;
                    inputBuffer.clear();
                    // Disable alarm upon unlocking
                    buzzer.disable();
                    // Update SAFE state and display temperature and humidity data using the latest DHT data
                    std::string tempStr = "Temp:" + std::to_string(lastTemp) + " C";
                    std::string humStr  = "Hum:"  + std::to_string(lastHumidity) + " %";
                    I2cDisplay::getInstance().displaySafeAndDHT(tempStr, humStr);
                    std::cout << "[I2cDisplayHandle] Correct password, state switched to SAFE" << std::endl;
                } else {
                    std::cout << "[I2cDisplayHandle] Wrong password, please try again." << std::endl;
                    I2cDisplay::getInstance().displayWrongPassword();
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    I2cDisplay::getInstance().displayIntrusion();
                    inputBuffer.clear();
                }
            }
        //}
    }
}
