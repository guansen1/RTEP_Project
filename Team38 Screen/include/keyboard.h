#ifndef KEYBOARD_H
#define KEYBOARD_H

typedef void (*KeyCallback)(char key); // 定义回调函数类型

bool initKeyboard();
void keyboardLoop();
void cleanupKeyboard();
void setKeyCallback(KeyCallback callback);  // **设置按键回调**

#endif