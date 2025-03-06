#include "ADS1115.h"
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cstdio>

ADS1115::ADS1115(int i2c_fd, uint8_t address)
    : i2c_fd(i2c_fd), address(address) {}

bool ADS1115::init() {
    // 例如：向配置寄存器写入初始配置
    // 这里简化处理，假设总是成功
    return true;
}

bool ADS1115::configureComparator(uint16_t threshold, uint16_t config) {
    // 向阈值寄存器及配置寄存器写入数据以配置比较器
    // 实际代码需要根据 ADS1115 数据手册编写
    return true;
}

int16_t ADS1115::readConversion() {
    // 读取转换寄存器（地址 0x00）
    uint8_t reg = 0x00;
    if (write(i2c_fd, &reg, 1) != 1) {
        perror("Failed to write conversion register pointer");
        return -1;
    }
    uint8_t buf[2];
    if (read(i2c_fd, buf, 2) != 2) {
        perror("Failed to read conversion data");
        return -1;
    }
    int16_t result = (buf[0] << 8) | buf[1];
    return result;
}
