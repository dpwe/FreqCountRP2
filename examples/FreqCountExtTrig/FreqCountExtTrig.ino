#include "FreqCountRP2.h"

int inputPin = 27;  // Must be odd-numbered pin.
int triggerPin = 28;  // Cannot be pin 0.

void setup()
{
  Serial.begin(9600);
  FreqCountRP2.beginExtTrig(inputPin, triggerPin);
}

void loop()
{
  if (FreqCountRP2.available())
  {
    uint32_t frequency = FreqCountRP2.read();
    Serial.println(frequency);
  }
}
