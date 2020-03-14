#include <Si4703.h>
#include <Wire.h>

int RST  = 4;   // Reset Pin
int SDIO = A4;  // Serial Data I/O Pin
int SCLK = A5;  // Serial Clock Pin
int STC  = 3;   // Seek/Tune Complete Pin

Si4703 radio(RST, SDIO, SCLK, STC);

void setup()
{
  radio.powerUp();
  radio.setChannel(944);
  radio.setVolume(5);
}

void loop()
{

radio.writeGPIO(GPIO1, GPIO_High);  // turn LED2 ON
delay(300);

radio.writeGPIO(GPIO1, GPIO_Low);  // turn LED2 OFF
delay(300);

}
