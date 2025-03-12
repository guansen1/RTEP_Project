// dht.h
#ifndef DHT_H
#define DHT_H

#include "gpio/gpio.h"
#include <functional>
#include <thread>
#include <atomic>
#include <array>  // 新增：用于std::array

// 定义 DHT11 的数据结构
struct DHTReading {
    float humidity;
    float temp_celsius;
};

class DHT11 {
public:
    DHT11(GPIO& gpio);
    ~DHT11();

    void start();
    void stop();
    void registerCallback(std::function<void(const DHTReading&)> callback);
    
    // 将readData改为公有方法，以便测试
    bool readData(DHTReading& result);
    
    // 添加测试专用的平滑处理方法
    #ifdef TEST_SMOOTHING_ENABLED
    void testSmoothReadings(DHTReading& reading) {
        smoothReadings(reading);
    }
    #endif

private:
    void worker();
    void timerEvent();  

    bool checkResponse();
    uint8_t readByte();
    uint8_t readBit();
    void smoothReadings(DHTReading& reading); 

    GPIO& gpio;  
    std::function<void(const DHTReading&)> callback;
    std::thread workerThread;
    std::atomic<bool> running;
    int timerfd; 

    static const int HISTORY_SIZE = 5;
    std::array<float, HISTORY_SIZE> temp_history;
    std::array<float, HISTORY_SIZE> humidity_history;
    int history_index = 0;
    bool history_filled = false;
};

#endif  // DHT_H