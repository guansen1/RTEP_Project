// buzzer.cpp
#include "buzzer.h"
#include <iostream>

#include <unistd.h>
#include <string.h>
Buzzer::Buzzer(GPIO& gpio, int buzzerPin) : gpio(gpio), buzzerPin(buzzerPin), alarm_active(false) {}

Buzzer::~Buzzer() {
    stopAlarm();
}

void Buzzer::startAlarm() {
    if (alarm_active.load()) return; // 避免重复创建线程

    alarm_active = true;
    alarmThread = std::thread(&Buzzer::alarmLoop, this);
}


void Buzzer::stopAlarm() {
    alarm_active = false;
    if (alarmThread.joinable()) {
        alarmThread.join();  // 等待线程结束
    }
}


void Buzzer::alarmLoop() {
    int timerFd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerFd == -1) {
        std::cerr << "Failed to create timerfd\n";
        return;
    }
    struct itimerspec its;
    its.it_value.tv_sec =0;  // 首次触发时间
    its.it_value.tv_nsec = 500000000;
    its.it_interval.tv_sec = 0;  // 周期触发间隔
    its.it_interval.tv_nsec = 500000000;
    if (timerfd_settime(timerFd, 0, &its, nullptr) == -1) {
        std::cerr << "Failed to set timerfd\n";
        close(timerFd);
        return;
    }
    while (alarm_active) {
        uint64_t exp;
        ssize_t s = read(timerFd, &exp, sizeof(uint64_t));
        if (s != sizeof(uint64_t)) {
            std::cerr << "Failed to read timerfd: " << strerror(errno) << std::endl;
            continue;
        }
        beep(1000,500);
   }
}

void Buzzer::beep(int frequency, int duration) {
    int period = 1000000 / frequency; // period (us)
    int half_period = period / 2;

    for (int i = 0; i < (duration * 1000) / period; i++) {
        gpio.writeGPIO(BUZZER_IO, 1);
        usleep(half_period);
        gpio.writeGPIO(BUZZER_IO, 0);
        usleep(half_period);
    }
    std::cout << "报警中 " << strerror(errno) << std::endl;
}

