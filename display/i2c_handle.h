#ifndef I2C_HANDLE_H
#define I2C_HANDLE_H

#include "gpio/gpio.h"
#include "display/i2c_display.h"
#include "buzzer/buzzer.h"  // Include the buzzer module
#include "keyboard/keyboard.h"
class I2cDisplayHandle : public GPIO::GPIOEventCallbackInterface {
public:
    // Constructor receives a Buzzer reference
    I2cDisplayHandle(Buzzer &buzzerRef);
    virtual ~I2cDisplayHandle();

    // PIR event callback: Switch to INVASION on rising edge (no action on falling edge)
    virtual void handleEvent(const gpiod_line_event &event) override;

    // DHT data handling: Update latest data and refresh display when in SAFE state
    void handleDHT(float temp, float humidity);

    // Keyboard key handling: Record password input during INVASION state, unlock upon verification
    void handleKeyPress(char key);

private:
    enum class DisplayState { SAFE, INVASION };
    DisplayState state;
    std::string inputBuffer; // Store password input
    float lastTemp;
    float lastHumidity;
    Buzzer &buzzer;  // Injected via constructor, no global variable used
};

#endif // I2C_HANDLE_H
