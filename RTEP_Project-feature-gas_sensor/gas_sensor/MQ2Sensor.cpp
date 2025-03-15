#include "MQ2Sensor.h"

MQ2Sensor::MQ2Sensor(ADS1115* adc) : adc(adc) {}

int16_t MQ2Sensor::getSensorReading() {
    return adc->readConversion();
}
