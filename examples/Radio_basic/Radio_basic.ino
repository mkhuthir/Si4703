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
  Serial.begin(115200);   // Start Terminal Port
  radio.powerUp();        // Power Up Device
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
   Serial.print(" MHz | VOL:");
   Serial.print(radio.getVolume());
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
