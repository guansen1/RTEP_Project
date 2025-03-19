// dht.h
#ifndef DHT_H
#define DHT_H
#include <sys/timerfd.h>
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

private:
    void worker();
    void timerEvent();  // 新增：定时器事件处理函数

    bool readData(DHTReading& result);
    bool checkResponse();
    uint8_t readByte();
    uint8_t readBit();
    void smoothReadings(DHTReading& reading);  // 新增：平滑处理函数

    GPIO& gpio;  // GPIO 对象引用
    std::function<void(const DHTReading&)> callback;
    std::thread workerThread;
    std::atomic<bool> running;
    int timerfd;  // 新增：定时器文件描述符

    // 数据平滑处理相关变量
    static const int HISTORY_SIZE = 5;
    std::array<float, HISTORY_SIZE> temp_history;
    std::array<float, HISTORY_SIZE> humidity_history;
    int history_index = 0;
    bool history_filled = false;
};

#endif // DHT_H