#include "buzzer.h"

Buzzer::Buzzer(RPI_PWM& pwm):pwm(pwm){

}

Buzzer::~Buzzer() {
    disable();
}

void Buzzer::enable(int freq) {
    pwm.start(BUZZER_PWM_CHANNEL, freq);
    pwm.setDutyCycle(50);
}

void Buzzer::disable() {
    pwm.stop();
}
