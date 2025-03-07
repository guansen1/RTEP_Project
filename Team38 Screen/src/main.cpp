#include "keyboard.h"
#include <iostream>

using namespace std;

// **按键回调函数**
void handleKeyPress(char key) {
    static string input_buffer;
    static bool alarm_active = false;

    input_buffer.push_back(key);
    cout << "输入: " << key << endl;

    if (input_buffer.size() == 4) {
        if (input_buffer == "1234") {
            cout << "✅ 警报解除！" << endl;
            alarm_active = false;
        } else if (input_buffer == "ABCD") {
            cout << "🔒 布防成功！" << endl;
            alarm_active = true;
        } else {
            cout << "❌ 密码错误！" << endl;
        }
        input_buffer.clear();
    }
}

int main() {
    if (!initKeyboard()) {
        cerr << "❌ 键盘初始化失败！" << endl;
        return -1;
    }

    // **注册回调**
    setKeyCallback(handleKeyPress);

    cout << "🔹 矩阵键盘已启动..." << endl;
    keyboardLoop();  // **自动回调处理**

    cleanupKeyboard();
    return 0;
}