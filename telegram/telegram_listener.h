#ifndef TELEGRAM_LISTENER_H
#define TELEGRAM_LISTENER_H

#include <atomic>
#include <string>
#include <thread>

// **Forward declaration of the Buzzer class
class Buzzer;

class TelegramListener {
public:
    // Constructer
    TelegramListener(const std::string& token, const std::string& chatId, Buzzer& buzzer);
    ~TelegramListener();

    // Start a listener thread
    void start();
    // Stop listening threads
    void stop();

    // Check to see if it is in detection mode
    bool isDetectionMode() const;
    // Check if alarms are allowed
    bool isAlarmEnabled() const;
    
    void sendTemperatureData(float temperature, float humidity);
    

private:
    // monitor a loop
    void run();

    std::string token;
    std::string chatId;
    std::atomic_bool detectionMode; // Control data sending
    std::atomic_bool alarmEnabled;  // Controls whether the PIR triggers the buzzer
    std::atomic_bool running;
    std::thread listenerThread;
    Buzzer& buzzer; // **References to Buzzer instances**
};

#endif // TELEGRAM_LISTENER_H
