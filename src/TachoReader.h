#pragma once

#include <Arduino.h>

class TachoReader {
public:
    void begin(uint8_t pin, uint8_t pulsesPerRev = 2);
    void update();          // call periodically from core 1
    uint16_t getRPM();      // safe to call from core 0

    bool isEnabled() const { return _pin != 0xFF; }

private:
    uint8_t _pin = 0xFF;
    uint8_t _pulsesPerRev = 2;
    volatile uint32_t _pulseCount = 0;
    uint32_t _lastTime = 0;
    volatile uint16_t _rpm = 0;

    void _onPulse();

    static TachoReader *_instances[2];
    static uint8_t _instanceCount;
    uint8_t _index = 0;
    static void _isr0();
    static void _isr1();
};
