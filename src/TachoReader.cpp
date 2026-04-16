#include "TachoReader.h"

TachoReader *TachoReader::_instances[2] = {nullptr, nullptr};
uint8_t TachoReader::_instanceCount = 0;

void TachoReader::begin(uint8_t pin, uint8_t pulsesPerRev) {
    _pin = pin;
    _pulsesPerRev = pulsesPerRev;

    pinMode(_pin, INPUT_PULLUP);

    _index = _instanceCount;
    _instances[_instanceCount++] = this;

    void (*isr)() = (_index == 0) ? _isr0 : _isr1;
    attachInterrupt(digitalPinToInterrupt(_pin), isr, FALLING);

    _lastTime = millis();
}

void TachoReader::update() {
    uint32_t now = millis();
    uint32_t elapsed = now - _lastTime;

    if (elapsed < 500)
        return;

    noInterrupts();
    uint32_t count = _pulseCount;
    _pulseCount = 0;
    interrupts();

    _rpm = (uint16_t)((uint32_t)count * 60000UL / (elapsed * _pulsesPerRev));
    _lastTime = now;
}

uint16_t TachoReader::getRPM() {
    return _rpm;
}

void TachoReader::_onPulse() {
    _pulseCount++;
}

void TachoReader::_isr0() { if (_instances[0]) _instances[0]->_onPulse(); }
void TachoReader::_isr1() { if (_instances[1]) _instances[1]->_onPulse(); }
