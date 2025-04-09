#include <iostream>
#include <thread>
#include <chrono>
#include "gpio/gpio.h"
#include "pir/pir.h"
#include "dht/dht.h"
#include "display/i2c_display.h"
#include "display/i2c_handle.h"
#include "buzzer/buzzer.h"
#include "keyboard/keyboard.h"  
#include "telegram/telegram.h"
#include "telegram/telegram_listener.h"

// define Telegram config
const std::string TELEGRAM_TOKEN = "7415933593:AAH3hw9NeuMsAOeuqyUZe_l935KP2mxaYGA";
const std::string TELEGRAM_CHAT_ID = "7262701565";

int main() {
    std::cout << "System Starting..." << std::endl;
    
    // init I2C display
    I2cDisplay::getInstance().init();
    
    // init GPIO 
    GPIO gpio;
    gpio.gpio_init();

    RPI_PWM pwm;
    Buzzer buzzer(pwm);
    
    // start Telegram 
    TelegramListener telegramListener(TELEGRAM_TOKEN, TELEGRAM_CHAT_ID, buzzer);
    telegramListener.start();
    

    // PIR event handler
    PIREventHandler pirHandler(gpio, buzzer, telegramListener);
    gpio.registerCallback(PIR_IO, &pirHandler);

    // i2c event handler
    I2cDisplayHandle displayHandle(buzzer);
    gpio.registerCallback(PIR_IO, &displayHandle);
    
    ActiveKeyboardScanner keyboardScanner(gpio,displayHandle);
    // start gpio event thread
    gpio.start();

    // init DHT11 sensor
    DHT11 dht11(gpio);
    dht11.registerCallback([&displayHandle, &telegramListener](const DHTReading &reading) {
        
        displayHandle.handleDHT(reading.temp_celsius, reading.humidity);
        
        telegramListener.sendTemperatureData(reading.temp_celsius, reading.humidity);
    });
    dht11.start();

    // init keyboard
    keyboardScanner.initkeyboard(displayHandle);
    keyboardScanner.start();  

    
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    telegramListener.stop();
    gpio.stop();  
    return 0;
}
