#ifndef dpwe_FreqCountRP2_h
#define dpwe_FreqCountRP2_h

#include <Arduino.h>

class _FreqCountRP2
{
private:
    uint8_t mPin;
    uint8_t mTriggerPin;
    struct repeating_timer mTimer;

    void _setup_pwm_counter(uint8_t freqPin);

public:
    static volatile uint8_t sIsFrequencyReady;
    static volatile uint32_t sFrequency;
    static volatile uint32_t sCount;
    static volatile uint32_t sLastCount;
    static uint8_t sSliceNum;

    _FreqCountRP2();
    ~_FreqCountRP2();

    // Frequency counter using internal interval timer.
    void beginTimer(uint8_t pin, uint16_t timerMs);
    // Frequency counter using external trigger pulse input.
    void beginExtTrig(uint8_t pin, uint8_t extTriggerPin);
    uint32_t read();
    uint8_t available();
    void end();
};

extern _FreqCountRP2 FreqCountRP2;

#endif // dpwe_FreqCountRP2_h
