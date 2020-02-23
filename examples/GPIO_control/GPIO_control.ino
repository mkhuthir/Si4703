#include <Si4703.h>
#include <Wire.h>

int resetPin  = 4;
int SDIO      = A4;
int SCLK      = A5;
int STC       = 3;

Si4703 radio(resetPin, SDIO, SCLK, STC);

void setup()
{
  radio.powerOn();
  radio.setChannel(944);
  radio.setVolume(15);
}

void loop()
{

radio.writeGPIO(GPIO1, GPIO_High);  // turn LED2 ON
delay(300);

radio.writeGPIO(GPIO1, GPIO_Low);  // turn LED2 OFF
delay(300);

}
