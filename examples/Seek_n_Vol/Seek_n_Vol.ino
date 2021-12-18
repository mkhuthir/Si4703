#include <Si4703.h>
#include <Wire.h>


Si4703 radio;             // using default values for all settings

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

    if (ch == 'n')            // Channel Seek next
      {
        if (!radio.seekUp())
          Serial.println("| Error: Seek failure or band limit reached!!");
        else
          displayInfo();
      }
    else if (ch == 'l')       // Channel Seek last
      {
        if (!radio.seekDown())
          Serial.println("| Error: Seek failure or band limit reached!!");
        else
          displayInfo();
      }
    else if (ch == '+')       // Increment Volume
      {
        radio.incVolume();
        displayInfo();
      } 
    else if (ch == '-')       // Decrement Volume
      {
        radio.decVolume();
        displayInfo();
      }
    else                      // Uknown command
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
  Serial.println("n l  Seek Up/Down  ");
  Serial.println("-----------------------");
  Serial.println("Select a key.\n");
}
