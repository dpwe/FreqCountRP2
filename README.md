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

* [FreqCountESP](https://github.com/karparan/FreqCountESP)
* [FreqCount library for Arduino & Teensy boards](https://www.pjrc.com/teensy/td_libs_FreqCount.html)

## License <a name="license"></a>

[MIT License](https://github.com/dpwe/FreqCountRP2/blob/master/LICENSE)

Copyright (c) 2022 [Dan Ellis](https://github.com/dpwe)
