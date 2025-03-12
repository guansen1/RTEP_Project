#include "pir.h"

PIREventHandler::PIREventHandler(GPIO&gpio) : gpio(gpio) {

}

PIREventHandler::~PIREventHandler() {
    stop();
}

void PIREventHandler::start() {
   
}

void PIREventHandler::stop() {
    
}

void PIREventHandler::handleEvent(const gpiod_line_event& event) {
    if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
        std::cout << "[PIR] Triggered！（RISING_EDGE）\n";
    } else if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
        std::cout << "[PIR] Trigger gone！（FALLING_EDGE）\n";
    }
}
