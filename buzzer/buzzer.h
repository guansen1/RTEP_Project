#ifndef BUZZER_H
#define BUZZER_H

#include <sys/timerfd.h>
#include "gpio/gpio.h"
#include <atomic>
#include <thread>
#include <chrono>

class Buzzer {
public:
    Buzzer(GPIO& gpio, int buzzerPin);
    ~Buzzer();

    void startAlarm();  // 启动报警（线程 + Timer）
    void stopAlarm();   // 停止报警（终止 Timer）
    
private:
    void alarmLoop();   // Timer 控制的响铃逻辑
    void beep(int frequency, int duration);
    GPIO& gpio;
    int buzzerPin;
    std::atomic<bool> alarm_active;
    std::thread alarmThread;
};

#endif // BUZZER_H
