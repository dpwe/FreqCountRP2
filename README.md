# FreqCountRP2
## A frequency counter library for the RP2040

### Table of Contents

- [About](#about)
- [Installation](#installation)
- [Usage](#usage)
- [Credits](#credits)
- [License](#license)

## About

A frequency counter library for RP2040. It counts the numbers of pulses on a specified pin during a fixed time frame using native interrupts and timers. It only supports one instance per sketch for now.

The API (and documentation) are copied from [FreqCountESP](http://github.com/kapraran/FreqCountESP) which provides the same functionality for the ESP32 platform.

## Frequency Counting on the RP2040

At first glance, the RP2040 chip does not appear to have a frequency counter peripheral.
However, this code achieves that functionality via the PWM system.
The PWM channels include 16 bit counters driven by the 125 MHz system clock.
The clock inputs can be gated via a selectable GPIO input pin.  
Simple level-based gating allows measuring the pulse width of input pulses/PWM 
(in units of the system clock).
However, the gating also provides "rising-edge" and "falling-edge" gating
(see fig 103 on page 524 of the 
[RP2040 Data Sheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf#page525)
).
It's not immediately clear what this means, but it appears to be that when the 
specified edge occurs at the input pin, exactly one pulse from the 125 MHz system clock
is fed through to the counter.  Thus, this configuration is effectively a pulse
counter, with the limitation that input edges are "synchronous" at 125 MHz (meaning
input edges will be missed if they occur faster than this).

I was actually surprised this works, but it does, certainly up to 10 MHz input.

## Installation

To use this library you have to import it into Arduino IDE as follows:
1. Download or clone this repository as a zip file.
2. Go to "Sketch" > "Include Library" > "Add .ZIP library..." and select the zip file to import it.

## Limitations

Due to hardware limitations, the frequency input pin must always be an **odd-numbered GPIO**.

Due to (needless) software limitations, GPIO 0 cannot be used as the trigger input pin.

## Usage

#### Include the library in the sketch

```C++
#include "FreqCountRP2.h"
```

#### Initialize the instance

When using the internal time, set which pin to use and the length of the time frame in milliseconds.

```C++
void setup()
{
  int inputPin = 5;  // Must be odd-numbered.
  int timerMs = 1000;
  FreqCountRP2.beginTimer(inputPin, timerMs);
}
```

When using an external time reference (for instance, the PPS output of a GPS receiver), specify the pins for the frequency input, and the external trigger input:

```C++
void setup()
{
  int inputPin = 5;
  int triggerPin = 6;
  FreqCountRP2.beginExtTrig(inputPin, triggerPin);
}
```


#### Read the frequency

Wait for a new value to become available and read it by calling the `read()` method.

```C++
void loop()
{
  if (FreqCountRP2.available())
  {
    uint32_t frequency = FreqCountRP2.read();
    // Do something with the frequency...
  }
}
```

## Credits <a name="credits"></a>

* [FreqCountESP](https://github.com/kapraran/FreqCountESP)
* [FreqCount library for Arduino & Teensy boards](https://www.pjrc.com/teensy/td_libs_FreqCount.html)

## License <a name="license"></a>

[MIT License](https://github.com/dpwe/FreqCountRP2/blob/master/LICENSE)

Copyright (c) 2022 [Dan Ellis](https://github.com/dpwe)
