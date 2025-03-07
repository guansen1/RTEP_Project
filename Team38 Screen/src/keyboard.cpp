#include "keyboard.h"
#include <iostream>
#include <gpiod.h>
#include <thread>
#include <vector>

using namespace std;

static struct gpiod_chip* chip = nullptr;
static struct gpiod_line_bulk colLines;  
static struct gpiod_line* rowLines[4];

const int rowPins[4] = {4,17,27,22};  
const int colPins[4] = {5,6,13,19};    

static const char keyMap[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

// **回调函数指针**
static KeyCallback key_callback = nullptr;

// **设置回调函数**
void setKeyCallback(KeyCallback callback) {
    key_callback = callback;
}

// **初始化 GPIO**
bool initKeyboard() {
    chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) return false;

    for (int i = 0; i < 4; ++i) {
        rowLines[i] = gpiod_chip_get_line(chip, rowPins[i]);
        if (!rowLines[i] || gpiod_line_request_output(rowLines[i], "keyboard", 0) < 0) {
            cerr << "Row GPIO init failed: " << rowPins[i] << endl;
            return false;
        }
    }

    vector<struct gpiod_line*> colTempLines;
    for (int i = 0; i < 4; ++i) {
        struct gpiod_line* line = gpiod_chip_get_line(chip, colPins[i]);
        if (!line || gpiod_line_request_falling_edge_events(line, "keyboard") < 0) {
            cerr << "Column GPIO event request failed: " << colPins[i] << endl;
            return false;
        }
        colTempLines.push_back(line);
    }

    gpiod_line_bulk_init(&colLines);
    gpiod_line_bulk_add_lines(&colLines, colTempLines.data(), colTempLines.size());

    return true;
}

// **扫描按键**
char determineKey(int col) {
    for (int row = 0; row < 4; ++row) {
        gpiod_line_set_value(rowLines[row], 1);
        this_thread::sleep_for(chrono::milliseconds(1));

        if (gpiod_line_get_value(colLines.lines[col]) == 1) {
            gpiod_line_set_value(rowLines[row], 0);
            return keyMap[row][col];
        }
        gpiod_line_set_value(rowLines[row], 0);
    }
    return '\0';
}

// **事件监听循环**
void keyboardLoop() {
    cout << "🔹 等待按键..." << endl;
    struct gpiod_line_event event;

    while (true) {
        int ret = gpiod_line_event_wait_bulk(&colLines, NULL);
        if (ret > 0) {
            for (int i = 0; i < colLines.num_lines; ++i) {
                struct gpiod_line* line = colLines.lines[i];

                if (gpiod_line_event_read(line, &event) == 0) {
                    if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
                        int col = -1;
                        for (int j = 0; j < 4; ++j) {
                            if (line == colLines.lines[j]) {
                                col = j;
                                break;
                            }
                        }
                        if (col != -1) {
                            char key = determineKey(col);
                            if (key != '\0' && key_callback) {
                                key_callback(key);  // **回调函数触发**
                            }
                        }
                    }
                }
            }
        }
    }
}

// **释放资源**
void cleanupKeyboard() {
    for (int i = 0; i < 4; i++) {
        gpiod_line_release(rowLines[i]);
    }
    for (int i = 0; i < colLines.num_lines; i++) {
        gpiod_line_release(colLines.lines[i]);
    }
    if (chip) gpiod_chip_close(chip);
}