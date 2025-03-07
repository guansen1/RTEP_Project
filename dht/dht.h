// dht.h
#ifndef DHT_H
#define DHT_H

#include "gpio/gpio.h"
#include <functional>
#include <thread>
#include <atomic>

// 定义 DHT11 的数据结构
struct DHTReading {
    float humidity;
    float temp_celsius;
};

class DHT11 {
public:
    DHT11(GPIO&gpio);
    ~DHT11();

    void start();
    void stop();
    void registerCallback(std::function<void(const DHTReading&)> callback);

private:
    void worker();

    bool readData(DHTReading& result);
    bool checkResponse();
    uint8_t readByte();
    uint8_t readBit();

    GPIO& gpio;  // GPIO 对象引用
    std::function<void(const DHTReading&)> callback;
    std::thread workerThread;
    std::atomic<bool> running;
};

#endif // DHT_H