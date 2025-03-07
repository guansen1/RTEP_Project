// dht.cpp
#include "dht.h"
#include <chrono>
#include <iostream>

DHT11::DHT11(GPIO&gpio) : gpio(gpio), callback(nullptr) {

}

DHT11::~DHT11() {
    stop();
}

void DHT11::start() {
    running = true;
    workerThread = std::thread(&DHT11::worker, this);
}

void DHT11::stop() {
    running = false;
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void DHT11::registerCallback(std::function<void(const DHTReading&)> callback) {
    this->callback = callback;
}

void DHT11::worker() {
    while (running) {
        DHTReading reading;
        if (readData(reading)) {
            std::cout << " DHT TEMP" <<reading.temp_celsius<< " DHT HUM" <<reading.humidity<<std::endl;
            if (callback) {
                callback(reading);  // 调用回调函数
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));  // 阻塞式延时 2 秒
    }
}

bool DHT11::readData(DHTReading& result) {
    gpio.configGPIO(DHT_IO, OUTPUT);
    gpio.writeGPIO(DHT_IO, 0);  // 拉低引脚
    std::this_thread::sleep_for(std::chrono::milliseconds(20));  // 保持低电平至少 18ms
    std::cout << "GPIO pull down finished " << std::endl;
    gpio.writeGPIO(DHT_IO, 1);  // 拉高引脚
    std::this_thread::sleep_for(std::chrono::microseconds(30));  // 保持高电平 20~40us
    std::cout << "GPIO pull up finished " << std::endl;
    // 检查 DHT11 响应
    if (!checkResponse()) {
        return false;
    }
    std::cout << "Checkpass " << std::endl;
    // 读取 40 位数据
    uint8_t data[5] = {0};
    for (int i = 0; i < 5; i++) {
        data[i] = readByte();
    }
    std::cout << "Data read from DHT11: ";
    for (int i = 0; i < 5; i++) {
        std::cout << "0x" << std::hex << static_cast<int>(data[i]) << " ";  // 以十六进制打印
    }
    std::cout << std::endl;
    
    if ((data[0] + data[1] + data[2] + data[3]) == data[4]) {
        result.humidity = data[0] + data[1]/10.0f;  // 处理湿度数据
        result.temp_celsius = data[2] + data[3]/10.0f;  // 处理温度数据
        return true;
    }
    std::cerr << "Checksum error: " << static_cast<int>(data[0] + data[1] + data[2] + data[3]) 
              << " != " << static_cast<int>(data[4]) << std::endl;
    return false;
 
}

bool DHT11::checkResponse() {
    gpio.configGPIO(DHT_IO, INPUT);  // 设置引脚为输入模式

    // 等待 DHT11 拉低引脚 40~80us
    auto start = std::chrono::steady_clock::now();
    while (gpio.readGPIO(DHT_IO) == 1) {
        if (std::chrono::steady_clock::now() - start > std::chrono::microseconds(100)) {
            std::cerr << "DHT pull down failed " << std::endl;
            return false;  // 超时
        }
    }
    std::cout << "DHT pull down finished " << std::endl;
    // 等待 DHT11 拉高引脚 40~80us
    start = std::chrono::steady_clock::now();
    while (gpio.readGPIO(DHT_IO) == 0) {
        if (std::chrono::steady_clock::now() - start > std::chrono::microseconds(100)) {
            std::cerr << "DHT pull up failed " << std::endl;
            return false;  // 超时
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