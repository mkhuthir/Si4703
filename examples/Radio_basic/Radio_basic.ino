#include <Si4703.h>
#include <Wire.h>

int RST  = 4;   // Reset Pin
int SDIO = A4;  // Serial Data I/O Pin
int SCLK = A5;  // Serial Clock Pin
int STC  = 3;   // Seek/Tune Complete Pin

Si4703 radio(RST, SDIO, SCLK, STC);

void setup()
{
  Serial.begin(115200);   // Start Terminal Port
  radio.powerUp();        // Power Up Device
  radio.setVolume(5);     // Set initial volume 5
  radio.setChannel(944);  // Set initial frequency 94.4 Mhz
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
   Serial.print("Channel:"); Serial.print(radio.getChannel()); 
   Serial.print(" Volume:"); Serial.println(radio.getVolume()); 
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
