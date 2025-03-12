#include "keyboard.h"

using namespace std;

Keyboard::Keyboard(GPIO& gpio) : gpio(gpio) {}

Keyboard::~Keyboard() {
    cleanup();
}

void Keyboard::init() {
    cout << "⌨️ 初始化键盘 GPIO..." << endl;
    for (int row : rowPins) {
        auto* handler = new KeyboardEventHandler(this);
        gpio.registerCallback(row, handler);
        handlers.push_back(handler);
    }
    for (int col : colPins) {
        auto* handler = new KeyboardEventHandler(this);
        gpio.registerCallback(col, handler);
        handlers.push_back(handler);
    }
}

void Keyboard::cleanup() {
    cout << "🔚 释放键盘 GPIO 资源" << endl;
    for (auto* handler : handlers) {
        delete handler;
    }
    handlers.clear();
}

void Keyboard::processKeyPress(int row, int col) {
    cout << "🔘 按键: " << keyMap[row][col] << endl;
}

KeyboardEventHandler::KeyboardEventHandler(Keyboard* parent) : parent(parent) {}

void KeyboardEventHandler::handleEvent(const gpiod_line_event& event) {
    static bool keyDetected = false;
    static auto lastPressTime = chrono::steady_clock::now();

    int pin = event.source.offset;
    int rowIndex = -1, colIndex = -1;

    // 检测行
    for (int i = 0; i < 4; i++) {
        if (rowPins[i] == pin) {
            rowIndex = i;
            break;
        }
    }

    // 检测列
    for (int i = 0; i < 4; i++) {
        if (colPins[i] == pin) {
            colIndex = i;
            break;
        }
    }

    // 行列触发后确认按键
    if (rowIndex != -1 && colIndex != -1 && !keyDetected) {
        auto now = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::milliseconds>(now - lastPressTime).count() > 50) { // 去抖
            parent->processKeyPress(rowIndex, colIndex);
            keyDetected = true;
            lastPressTime = now;
        }
    }

    // 按键松开时重置状态
    if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
        keyDetected = false;
    }
}
