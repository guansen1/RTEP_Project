// dht.cpp
#include "dht.h"
#include <chrono>
#include <iostream>

#include <unistd.h>
#include <string.h>

DHT11::DHT11(GPIO& gpio) : gpio(gpio), callback(nullptr), timerfd(-1) {

}

DHT11::~DHT11() {
    stop();
}

void DHT11::start() {
    running = true;
    // 创建定时器文件描述符
    timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd == -1) {
        std::cerr << "Failed to create timerfd: " << strerror(errno) << std::endl;
        return;
    }
    
    // 设置定时器，每2秒触发一次
    struct itimerspec its;
    its.it_value.tv_sec = 2;  // 首次触发时间
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 2;  // 周期触发间隔
    its.it_interval.tv_nsec = 0;
    
    if (timerfd_settime(timerfd, 0, &its, NULL) == -1) {
        std::cerr << "Failed to set timerfd: " << strerror(errno) << std::endl;
        close(timerfd);
        timerfd = -1;
        return;
    }
    
    // 启动工作线程
    workerThread = std::thread(&DHT11::worker, this);
}

void DHT11::stop() {
    running = false;
    
    // 关闭定时器文件描述符，这会使阻塞的read调用返回
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
        
        // 阻塞等待定时器触发
        uint64_t exp;
        ssize_t s = read(timerfd, &exp, sizeof(uint64_t));
        if (s != sizeof(uint64_t)) {
            if (running) {
                std::cerr << "读取定时器失败: " << strerror(errno) << std::endl;
            }
            continue;
        }
        
        // 定时器触发，执行DHT11读取
        timerEvent();
    }
}

void DHT11::timerEvent() {
    DHTReading reading;
    if (readData(reading)) {
        // 应用平滑处理
        smoothReadings(reading);
        if (callback) {
            callback(reading);  // 调用回调函数
        }
    }
}

bool DHT11::readData(DHTReading& result) {
    // 尝试最多3次读取
    for (int retry = 0; retry < 3; retry++) {
    gpio.configGPIO(DHT_IO, OUTPUT);
    gpio.writeGPIO(DHT_IO, 0);  // 拉低引脚
    std::this_thread::sleep_for(std::chrono::milliseconds(20));  // 保持低电平至少 18ms
    gpio.writeGPIO(DHT_IO, 1);  // 拉高引脚
    std::this_thread::sleep_for(std::chrono::microseconds(30));  // 保持高电平 20~40us
    // 检查 DHT11 响应
    if (!checkResponse()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 等待一小段时间再重试
        continue;  // 尝试下一次
    }
    
    // 读取 40 位数据
    uint8_t data[5] = {0};
    for (int i = 0; i < 5; i++) {
        data[i] = readByte();
    }
    
    if ((data[0] + data[1] + data[2] + data[3]) == data[4]) {
        float humidity = data[0] + data[1]/10.0f;  // 处理湿度数据
        float temp_celsius = data[2] + data[3]/10.0f;  // 处理温度数据
        
        // 简单的合理性检查
        if (humidity >= 0 && humidity <= 100 && 
            temp_celsius >= -10 && temp_celsius <= 50) {
            result.humidity = humidity;
            result.temp_celsius = temp_celsius;
            return true;
        } 
    } 

     // 如果校验失败或数据不合理，等待短暂时间后重试
     std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return false;  // 3次尝试都失败
}

bool DHT11::checkResponse() {
    gpio.configGPIO(DHT_IO, INPUT);  // 设置引脚为输入模式

    // 等待 DHT11 拉低引脚 40~80us
    auto start = std::chrono::steady_clock::now();
    while (gpio.readGPIO(DHT_IO) == 1) {
        if (std::chrono::steady_clock::now() - start > std::chrono::microseconds(100)) {
            return false;  // 超时
        }
    }
    // 等待 DHT11 拉高引脚 40~80us
    start = std::chrono::steady_clock::now();
    while (gpio.readGPIO(DHT_IO) == 0) {
        if (std::chrono::steady_clock::now() - start > std::chrono::microseconds(100)) {
            return false;  // 超时
        }
    }
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
    // 等待低电平结束
    auto start = std::chrono::steady_clock::now();
    while (gpio.readGPIO(DHT_IO) == 0) {
        if (std::chrono::steady_clock::now() - start > std::chrono::microseconds(100)) {
            return 0;  // 超时
        }
    }

    // 测量高电平持续时间
    start = std::chrono::steady_clock::now();
    while (gpio.readGPIO(DHT_IO) == 1) {
        if (std::chrono::steady_clock::now() - start > std::chrono::microseconds(100)) {
            return 0;  // 超时
        }
    }
    
    // 计算高电平持续时间
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start).count();
        
    // 如果高电平持续时间大于约50us，则为1，否则为0
    return (duration > 50) ? 1 : 0;
}

// 在dht.cpp文件中添加此函数
void DHT11::smoothReadings(DHTReading& reading) {
    // 保存当前读数到历史数组
    temp_history[history_index] = reading.temp_celsius;
    humidity_history[history_index] = reading.humidity;
    
    history_index = (history_index + 1) % HISTORY_SIZE;
    if (history_index == 0) history_filled = true;
    
    // 如果历史数据已填充，计算平均值
    if (history_filled) {
        float temp_sum = 0;
        float humidity_sum = 0;
        
        for (int i = 0; i < HISTORY_SIZE; i++) {
            temp_sum += temp_history[i];
            humidity_sum += humidity_history[i];
        }
        
        reading.temp_celsius = temp_sum / HISTORY_SIZE;
        reading.humidity = humidity_sum / HISTORY_SIZE;
    }
}
