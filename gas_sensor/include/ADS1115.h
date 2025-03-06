#ifndef ADS1115_H
#define ADS1115_H

#include <cstdint>

class ADS1115 {
public:
    // 构造函数，传入 I2C 文件描述符和设备地址（默认为 0x48）
    ADS1115(int i2c_fd, uint8_t address = 0x48);
    
    // 初始化 ADC，例如配置寄存器
    bool init();
    
    // 配置比较器模式（可选，用于产生中断）
    bool configureComparator(uint16_t threshold, uint16_t config);
    
    // 从转换寄存器读取 ADC 数据
    int16_t readConversion();

private:
    int i2c_fd;
    uint8_t address;
};

#endif // ADS1115_H
