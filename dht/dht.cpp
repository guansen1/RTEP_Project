// dht.cpp
#include "dht.h"
#include <chrono>
#include <iostream>
#include <sys/timerfd.h>
#include <unistd.h>
#include <string.h>

DHT11::DHT11(GPIO& gpio) : gpio(gpio), callback(nullptr), timerfd(-1) {

}

DHT11::~DHT11() {
    stop();
}

void DHT11::start() {
    running = true;
    // create timerfd
    timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd == -1) {
        std::cerr << "创建定时器失败: " << strerror(errno) << std::endl;
        return;
    }
    
    // set timer param
    struct itimerspec its;
    its.it_value.tv_sec = 2;  // first trigger time
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 2;  // periodical trigger time
    its.it_interval.tv_nsec = 0;
    
    if (timerfd_settime(timerfd, 0, &its, NULL) == -1) {
        std::cerr << "set timer failed: " << strerror(errno) << std::endl;
        close(timerfd);
        timerfd = -1;
        return;
    }
    
    // start timer thread
    workerThread = std::thread(&DHT11::worker, this);
}

void DHT11::stop() {
    running = false;
    
    // close timerfd, stop read trigger
    if (timerfd != -1) {
        close(timerfd);
        timerfd = -1;
    }
    
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void DHT11::registerCallback(std::function<void(const DHTReading&)> callback) {
    this->callback = callback;
}

void DHT11::worker() {
    while (running) {
        if (timerfd == -1) {
            break;
        }
        
        // block until timer triggered
        uint64_t exp;
        ssize_t s = read(timerfd, &exp, sizeof(uint64_t));
        if (s != sizeof(uint64_t)) {
            if (running) {
                std::cerr << "read content err: " << strerror(errno) << std::endl;
            }
            continue;
        }
        
        // reach timer, trigger to read
        timerEvent();
    }
}

void DHT11::timerEvent() {
    DHTReading reading;
    if (readData(reading)) {
        // smooth reading result
        smoothReadings(reading);
        
        std::cout << " DHT TEMP " << reading.temp_celsius << " DHT HUM " << reading.humidity << std::endl;
        if (callback) {
            callback(reading);  //execute callback
        }
    }
}

bool DHT11::readData(DHTReading& result) {
    // 3 times attempt
    for (int retry = 0; retry < 3; retry++) {
        gpio.configGPIO(DHT_IO, OUTPUT);
        gpio.writeGPIO(DHT_IO, 0);  // active pull down
        std::this_thread::sleep_for(std::chrono::milliseconds(20));  // stay low 
        gpio.writeGPIO(DHT_IO, 1);  // active pull up
        std::this_thread::sleep_for(std::chrono::microseconds(30));  // stay high
        // check rsp of DHT11
        if (!checkResponse()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));  // wait then retry
            continue;  //start next for loop
        }

        uint8_t data[5] = {0};
        for (int i = 0; i < 5; i++) {
            data[i] = readByte();
        }
        
        if ((data[0] + data[1] + data[2] + data[3]) == data[4]) {
            float humidity = data[0] + data[1]/10.0f;  // calculate temp data
            float temp_celsius = data[2] + data[3]/10.0f;  // calculate humid data            
            // rst logic check
            if (humidity >= 0 && humidity <= 100 && 
                temp_celsius >= -10 && temp_celsius <= 50) {
                result.humidity = humidity;
                result.temp_celsius = temp_celsius;
                return true;
            } 
        } 
        // err data retry for loop
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }    
    return false;  
}

bool DHT11::checkResponse() {
    gpio.configGPIO(DHT_IO, INPUT);  //set io to input

    //wait pull down rsp 40~80us
    auto start = std::chrono::steady_clock::now();
    while (gpio.readGPIO(DHT_IO) == 1) {
        if (std::chrono::steady_clock::now() - start > std::chrono::microseconds(100)) {
            std::cerr << "DHT pull down failed " << std::endl;
            return false;  // timeout
        }
    }
    std::cout << "DHT pull down finished " << std::endl;
    //wait pull up rsp 40~80us
    start = std::chrono::steady_clock::now();
    while (gpio.readGPIO(DHT_IO) == 0) {
        if (std::chrono::steady_clock::now() - start > std::chrono::microseconds(100)) {
            std::cerr << "DHT pull up failed " << std::endl;
            return false;  // timeout
        }
    }
    std::cout << "DHT pull up finished " << std::endl;
    return true;
}

uint8_t DHT11::readByte() {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte <<= 1;
        byte |= readBit();
    }
    return byte;
}

uint8_t DHT11::readBit() {
    // wait until low level end
    auto start = std::chrono::steady_clock::now();
    while (gpio.readGPIO(DHT_IO) == 0) {
        if (std::chrono::steady_clock::now() - start > std::chrono::microseconds(100)) {
            return 0;  // timeout
        }
    }

    // wait until high level end
    start = std::chrono::steady_clock::now();
    while (gpio.readGPIO(DHT_IO) == 1) {
        if (std::chrono::steady_clock::now() - start > std::chrono::microseconds(100)) {
            return 0;  // timeout
        }
    }
    
    // calculate high level hold time
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start).count();
        
    // hold time > 50us -- logic 1 
    return (duration > 50) ? 1 : 0;
}

void DHT11::smoothReadings(DHTReading& reading) {
    // save to history arry
    temp_history[history_index] = reading.temp_celsius;
    humidity_history[history_index] = reading.humidity;
    
    history_index = (history_index + 1) % HISTORY_SIZE;
    if (history_index == 0) history_filled = true;
    
    // reach arry limit
    if (history_filled) {
        float temp_sum = 0;
        float humidity_sum = 0;
        
        for (int i = 0; i < HISTORY_SIZE; i++) {
            temp_sum += temp_history[i];
            humidity_sum += humidity_history[i];
        }
        
        reading.temp_celsius = temp_sum / HISTORY_SIZE;
        reading.humidity = humidity_sum / HISTORY_SIZE;
        
        std::cout << "Smooth Temp=" << reading.temp_celsius 
                  << "°C, Smooth Humid=" << reading.humidity << "%" << std::endl;
    }
}