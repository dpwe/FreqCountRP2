#include "FreqCountRP2.h"

int inputPin = 27;  // Must be odd-numbered pin.
int timerMs = 1000;

void setup()
{
  Serial.begin(9600);
  FreqCountRP2.beginTimer(inputPin, timerMs);
}

void loop()
{
  if (FreqCountRP2.available())
  {
    uint32_t frequency = FreqCountRP2.read();
    Serial.println(frequency);
  }
}
