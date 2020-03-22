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
  Serial.begin(115200);   // Start Terminal Port
  radio.start();          // Power Up Device
  radio.setChannel(9440); // Set initial frequency 94.4 Mhz
  radio.setVolume(1);     // Set initial volume
  
  displayHelp();          // Show Help Message
  displayInfo();          // Show current settings info

}

void loop()
{
  if (Serial.available())
  {
    char ch = Serial.read();
    Serial.print(" ");

    if (ch == 'u') 
    {
      radio.incChannel();
      displayInfo();
    } 
    else if (ch == 'd') 
    {
      radio.decChannel();
      displayInfo();
    } 
    else if (ch == '+') 
    {
      radio.incVolume();
      displayInfo();
    } 
    else if (ch == '-') 
    {
      radio.decVolume();
      displayInfo();
    }
    else
    {
      Serial.println("Unknown Key !!");
      displayHelp();
    }
    
  }
}

void displayInfo()
{
  Serial.print("| Ch:");
  Serial.print(float(radio.getChannel())/100,2);
  Serial.print(" MHz | RSSI:");
  Serial.print(radio.getRSSI());
  Serial.print(" | VOL:");
  Serial.print(radio.getVolume());
  Serial.print(" | VolExt:");
  Serial.print(radio.getVolExt());
  Serial.print(" | ST:");
  Serial.print(radio.getST());
  Serial.print(" | DMute:");
  Serial.print(radio.getMute());
  Serial.print(" | DMono:");
  Serial.print(radio.getMono());
  Serial.println(" |");

}

void displayHelp()
{
  Serial.println("\nUse below keys to control");
  Serial.println("-----------------------");
  Serial.println("+ -  Volume +/- (max 15)");
  Serial.println("u d  Frequency Up/Down  ");
  Serial.println("-----------------------");
  Serial.println("Select a key.\n");
}
