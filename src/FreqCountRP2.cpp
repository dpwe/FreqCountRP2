#include "FreqCountRP2.h"

// See page 528 of https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"

volatile uint8_t _FreqCountRP2::sIsFrequencyReady = false;
volatile uint32_t _FreqCountRP2::sFrequency = 0;
volatile uint32_t _FreqCountRP2::sCount = 0;
volatile uint32_t _FreqCountRP2::sLastCount = 0;
uint8_t _FreqCountRP2::sSliceNum = 0;
uint8_t _FreqCountRP2::sTriggerPin = 0;

static void _on_pwm_wrap() {
    // Clear the interrupt flag that brought us here
    pwm_clear_irq(_FreqCountRP2::sSliceNum);
    // Wind on the underlying counter.
    _FreqCountRP2::sCount += (1L << 16);
}

static uint32_t _pwm_read(uint sliceNum) {
    uint32_t this_count = _FreqCountRP2::sCount;
    uint16_t part_count = pwm_get_counter(sliceNum);
    if (part_count < 100) {
        // Maybe it just rolled over?  Re-check base_count.
        this_count = _FreqCountRP2::sCount;
    }
    this_count += part_count;
    uint32_t advance = (this_count - _FreqCountRP2::sLastCount);  // Will handle wraparound correctly.
    _FreqCountRP2::sLastCount = this_count;
    return advance;
}

static void _on_trigger() {
    _FreqCountRP2::sFrequency = _pwm_read(_FreqCountRP2::sSliceNum);
    _FreqCountRP2::sIsFrequencyReady = true;
    gpio_acknowledge_irq(_FreqCountRP2::sTriggerPin, IO_IRQ_BANK0);
}

void _FreqCountRP2::_setup_pwm_counter(uint8_t freq_pin) {
    // Configure the PWM circuit to count pulses on freq_pin.
    // Only the PWM B pins can be used as inputs.
    assert(pwm_gpio_to_channel(freq_pin) == PWM_CHAN_B);
    mPin = freq_pin;
    sSliceNum = pwm_gpio_to_slice_num(freq_pin);

    // Count once for every rising edge on PWM B input
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv_mode(&cfg, PWM_DIV_B_RISING);
    pwm_config_set_clkdiv(&cfg, 1);
    pwm_init(sSliceNum, &cfg, false);
    gpio_set_function(freq_pin, GPIO_FUNC_PWM);
    pwm_set_enabled(sSliceNum, true);

    // Setup the wraparound interrupt.
    // Mask our slice's IRQ output into the PWM block's single interrupt line,
    // and register our interrupt handler
    pwm_clear_irq(sSliceNum);
    pwm_set_irq_enabled(sSliceNum, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, _on_pwm_wrap);
    irq_set_enabled(PWM_IRQ_WRAP, true);
}

void _FreqCountRP2::beginExtTrig(uint8_t freq_pin, uint8_t trig_pin) {
    _setup_pwm_counter(freq_pin);
    // Setup trigger interrupt.
    sTriggerPin = trig_pin;
    // Use lower-level access to send *any* gpio interrupt to the trigger handler for less latency.
    // Gived rock-solid counts, whereas using set_irq_enabled was bouncing around by up to 200 counts (20us).
    // see https://forums.raspberrypi.com/viewtopic.php?t=306132
    gpio_init(sTriggerPin); 
    gpio_set_dir(sTriggerPin, GPIO_IN);
    irq_set_exclusive_handler(IO_IRQ_BANK0, _on_trigger);
    gpio_set_irq_enabled(sTriggerPin, GPIO_IRQ_EDGE_RISE, true);
    irq_set_enabled(IO_IRQ_BANK0, true);
}

static bool _repeating_timer_callback(struct repeating_timer *t) {
    _FreqCountRP2::sFrequency = _pwm_read(_FreqCountRP2::sSliceNum);
    _FreqCountRP2::sIsFrequencyReady = true;
    return true;
}

void _FreqCountRP2::beginTimer(uint8_t freq_pin, uint16_t period_ms) {
    _setup_pwm_counter(freq_pin);
    // Setup regular timer interrupt.
    // Negative period specifies (negative of) delay between successive calls,
    // not between end of handler and next call.
    add_repeating_timer_ms(-period_ms, _repeating_timer_callback, NULL, &mTimer);
}

bool _FreqCountRP2::available(void) {
    return sIsFrequencyReady;
}

uint32_t _FreqCountRP2::read(void) {
    sIsFrequencyReady = false;
    return sFrequency;
}

_FreqCountRP2::_FreqCountRP2()
{
    sTriggerPin = 0;
}

_FreqCountRP2::~_FreqCountRP2()
{
    end();
}

void _FreqCountRP2::end()
{
    pwm_set_irq_enabled(sSliceNum, false);
    pwm_set_enabled(sSliceNum, false);
    irq_set_enabled(PWM_IRQ_WRAP, false);
    irq_remove_handler(PWM_IRQ_WRAP, _on_pwm_wrap);

    if(sTriggerPin == 0) {
        cancel_repeating_timer(&mTimer);
    } else {
        gpio_set_irq_enabled(sTriggerPin, GPIO_IRQ_EDGE_RISE, false);
        irq_set_enabled(IO_IRQ_BANK0, false);
        irq_remove_handler(IO_IRQ_BANK0, _on_trigger);
    }
}

_FreqCountRP2 FreqCountRP2;
