#include <Si4703.h>
#include <Wire.h>

Si4703 radio;             // using default values for all settings

void setup()
{
  radio.start();          // Power Up Device
  radio.setChannel(9440); // Set frequency 94.4 Mhz
  radio.setVolume(1);     // Set volume
}

void loop()
{
}
