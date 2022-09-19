#include "FreqCountRP2.h"

// See page 528 of https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"

volatile uint8_t _FreqCountRP2::sIsFrequencyReady = false;
volatile uint32_t _FreqCountRP2::sFrequency = 0;
volatile uint32_t _FreqCountRP2::sCount = 0;
volatile uint32_t _FreqCountRP2::sLastCount = 0;

uint _FreqCountRP2::mSliceNum = 0;

static void _on_pwm_wrap() {
  // Clear the interrupt flag that brought us here
  pwm_clear_irq(_FreqCountRP2::mSliceNum);
  // Wind on the underlying counter.
  _FreqCountRP2:sCount += (1L << 16);
}

static uint32_t _pwm_read(void) {
  uint32_t this_count = _FreqCountRP2::sCount;
  uint16_t part_count = pwm_get_counter(_FreqCountRP2::mSliceNum);
  if (part_count < 100) {
    // Maybe it just rolled over?  Re-check base_count.
    this_count = _FreqCountRP2::sCount;
  }
  this_count += part_count;
  uint32_t advance = (this_count - _FreqCount::sLastCount);  // Will handle wraparound correctly.
  _FreqCount::sLastCount = this_count;
  return advance;
}

static void _on_trigger() {
  _FreqCountRP2::sFrequency = _pwm_read();
  _FreqCountRP2::sIsFrequencyReady = true;
  gpio_acknowledge_irq(_FreqCountRP2::mTriggerPin, IO_IRQ_BANK0);
}

void FreqCountRP2::_setup_pwm_counter(uint freq_pin) {
    // Configure the PWM circuit to count pulses on freq_pin.
    // Only the PWM B pins can be used as inputs.
    assert(pwm_gpio_to_channel(freq_pin) == PWM_CHAN_B);
    this.mPin = freq_pin;
    this.mSliceNum = pwm_gpio_to_slice_num(freq_pin);

    // Count once for every rising edge on PWM B input
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv_mode(&cfg, PWM_DIV_B_RISING);
    pwm_config_set_clkdiv(&cfg, 1);
    pwm_init(this.mSliceNum, &cfg, false);
    gpio_set_function(freq_pin, GPIO_FUNC_PWM);
    pwm_set_enabled(this.mSliceNum, true);

    // Setup the wraparound interrupt.
    // Mask our slice's IRQ output into the PWM block's single interrupt line,
    // and register our interrupt handler
    pwm_clear_irq(this.mSliceNum);
    pwm_set_irq_enabled(this.mSliceNum, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, _on_pwm_wrap);
    irq_set_enabled(PWM_IRQ_WRAP, true);
}

void FreqCountRP2::beginExtTrig(uint freq_pin, uint trig_pin) {
    FreqCountRP2::_setup_pwm_counter(freq_pin);
    // Setup trigger interrupt.
    this.mTriggerPin = trig_pin;
    // Use lower-level access to send *any* gpio interrupt to the trigger handler for less latency.
    // Gived rock-solid counts, whereas using set_irq_enabled was bouncing around by up to 200 counts (20us).
    // see https://forums.raspberrypi.com/viewtopic.php?t=306132
    gpio_init(trig_pin); 
    gpio_set_dir(trig_pin, GPIO_IN);
    irq_set_exclusive_handler(IO_IRQ_BANK0, _on_trigger);
    gpio_set_irq_enabled(trig_pin, GPIO_IRQ_EDGE_RISE, true);
    irq_set_enabled(IO_IRQ_BANK0, true);
}

static bool _repeating_timer_callback(struct repeating_timer *t) {
  _FreqCountRP2::sFrequency = _pwm_read();
  _FreqCountRP2::sIsFrequencyReady = true;
  return true;
}

void FreqCountRP2::beginTimer(uint freq_pin, uint period_ms) {
    FreqCountRP2::_setup_pwm_counter(freq_pin);
    // Setup regular timer interrupt.
    // Negative period specifies (negative of) delay between successive calls,
    // not between end of handler and next call.
    add_repeating_timer_ms(-period_ms, repeating_timer_callback, NULL, &this.mTimer);
}

bool FreqCountRP2::available(void) {
  return last_period_valid;
}

uint32_t FreqCountRP2::read(void) {
  last_period_valid = false;
  return last_period;
}





void IRAM_ATTR onTimer()
{
  portENTER_CRITICAL_ISR(&_FreqCountRP2::sMux);
  _FreqCountRP2::sFrequency = _FreqCountRP2::sCount;
  _FreqCountRP2::sCount = 0;
  _FreqCountRP2::sIsFrequencyReady = true;
  portEXIT_CRITICAL_ISR(&_FreqCountRP2::sMux);
}
#endif // !USE_PCNT

portMUX_TYPE _FreqCountRP2::sMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onRise()
{
  portENTER_CRITICAL_ISR(&_FreqCountRP2::sMux);
  _FreqCountRP2::sCount++;
  portEXIT_CRITICAL_ISR(&_FreqCountRP2::sMux);
}

_FreqCountRP2::_FreqCountRP2()
{
  mTimer = NULL;
}

_FreqCountRP2::~_FreqCountRP2()
{
  end();
}

void _FreqCountRP2::_begin(uint8_t freqPin, uint8_t freqPinIOMode)
{
  // Configure counting on frequency input pin.
  mPin = freqPin;
  sIsFrequencyReady = false;
  sCount = 0;
  sFrequency = 0;

  pinMode(mPin, freqPinIOMode);

#ifdef USE_PCNT
  _FreqCountRP2::sLastPcnt = 0;
  mIsrHandle = setupPcnt(mPin, &_FreqCountRP2::sCount);
#else  // !USE_PCNT
  attachInterrupt(mPin, &onRise, RISING);
#endif  // USE_PCNT
  if(mTriggerPin == 0) {
    // Not external trigger, start internal timer.
    timerAlarmEnable(mTimer);
  }
}

void _FreqCountRP2::begin(uint8_t freqPin, uint16_t timerMs, uint8_t hwTimerId, uint8_t freqPinIOMode)
{
  // Count frequency using internal timer.
  // mTriggerPin == 0 means we're using internal timer.
  mTriggerPin = 0;
  mTimer = timerBegin(hwTimerId, 80, true);
  timerAttachInterrupt(mTimer, &onTimer, true);
  timerAlarmWrite(mTimer, timerMs * 1000, true);

  _begin(freqPin, freqPinIOMode);
}

void _FreqCountRP2::beginExtTrig(uint8_t freqPin, uint8_t extTriggerPin, uint8_t freqPinIOMode, uint8_t extTriggerMode)
{
  // Count frequency between events from an external trigger input.
  // mTriggerPin == 0 means we're using internal timer.
  assert(extTriggerPin > 0);
  mTriggerPin = extTriggerPin;
  pinMode(mTriggerPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(mTriggerPin), &onTimer, extTriggerMode);

  _begin(freqPin, freqPinIOMode);
}

uint32_t _FreqCountRP2::read()
{
  sIsFrequencyReady = false;
  return sFrequency;
}

uint8_t _FreqCountRP2::available()
{
  return sIsFrequencyReady;
}

void _FreqCountRP2::end()
{
#ifdef USE_PCNT
  teardownPcnt(mIsrHandle);
#else 
  detachInterrupt(mPin);
#endif
  if(mTriggerPin == 0) {
    timerAlarmDisable(mTimer);
    timerDetachInterrupt(mTimer);
    timerEnd(mTimer);
  } else {
    detachInterrupt(digitalPinToInterrupt(mTriggerPin));
  }
}

_FreqCountRP2 FreqCountRP2;
