#include <Si4703.h>
#include <Wire.h>

#define RST   4            // Reset Pin
#define SDIO  A4           // Serial Data I/O Pin
#define SCLK  A5           // Serial Clock Pin
#define INT   3            // Interrupt Pin
#define BAND  BAND_US_EU   // Select band frequency range
#define SPACE SPACE_100KHz // Select band spacing
#define DE    DE_75us      // Select de-emphasis

Si4703 radio(RST, SDIO, SCLK, INT, BAND, SPACE, DE);

void setup()
{
  radio.powerUp();
  radio.setChannel(9440);
  radio.setVolume(3);
}

void loop()
{

radio.writeGPIO(GPIO1, GPIO_High);  // turn LED2 ON
delay(300);

radio.writeGPIO(GPIO1, GPIO_Low);  // turn LED2 OFF
delay(300);

}
