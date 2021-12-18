#include <Si4703.h>
#include <Wire.h>

// Used Pins
#define RST   4            // Reset Pin
#define SDIO  A4           // Serial Data I/O Pin
#define SCLK  A5           // Serial Clock Pin
#define INT   3            // Interrupt Pin

// Select Region 
#define BAND  BAND_US_EU    // Select band frequency range
#define SPACE SPACE_100KHz  // Select band spacing
#define DE    DE_75us       // Select de-emphasis

// Seek Settings
#define SKMODE  SKMODE_STOP // Stop when reaching band limit
#define SEEKTH  00          // Seek RSSI Threshold
#define SKCNT   SKCNT_DIS   // Clicks Number Threshold
#define SKSNR   SKSNR_DIS   // Signal/Noise Ratio

Si4703 radio(RST, SDIO, SCLK, INT, BAND, SPACE, DE, SKMODE, SEEKTH, SKCNT, SKSNR);

void setup()
{
  radio.start();
  radio.setChannel(9440);
  radio.setVolume(3);
}

void loop()
{

  radio.writeGPIO(GPIO1, GPIO_High);  // turn LED1 ON
  radio.writeGPIO(GPIO2, GPIO_Low);   // turn LED2 OFF
  delay(300);

  radio.writeGPIO(GPIO1, GPIO_Low);   // turn LED1 OFF
  radio.writeGPIO(GPIO2, GPIO_High);  // turn LED2 ON
  delay(300);

}
