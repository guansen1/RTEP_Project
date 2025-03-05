#ifndef GPIO_H
#define GPIO_H

#include <wiringPi.h>

/**
 * @brief GPIO 管理类
 *
 * 封装了初始化、设置引脚模式、读写操作等常用功能。
 */
class Gpio {
public:
    /**
     * @brief 初始化 wiringPi 库
     *
     * @return true 初始化成功，false 初始化失败
     */
    static bool init();

    /**
     * @brief 将指定引脚设置为输入模式
     *
     * @param pin wiringPi 编号
     */
    static void setInput(int pin);

    /**
     * @brief 将指定引脚设置为输出模式
     *
     * @param pin wiringPi 编号
     */
    static void setOutput(int pin);

    /**
     * @brief 向指定引脚写入值
     *
     * @param pin wiringPi 编号
     * @param value 输出值（HIGH 或 LOW）
     */
    static void write(int pin, int value);

    /**
     * @brief 读取指定引脚的输入状态
     *
     * @param pin wiringPi 编号
     * @return int 返回 HIGH 或 LOW
     */
    static int read(int pin);
};

#endif // GPIO_H
