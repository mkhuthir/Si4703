#include <Si4703.h>
#include <Wire.h>

int resetPin  = 2;
int SDIO      = A4;
int SCLK      = A5;
int STC       = 3;

Si4703 radio(resetPin, SDIO, SCLK, STC);

void setup()
{
  Serial.begin(115200);
  Serial.println("\nWelcome");
  radio.powerOn();
  radio.setChannel(944);
  radio.setVolume(15);
}

void loop()
{
  if (Serial.available())
  {
    char ch = Serial.read();
    Serial.println(":");
   
    if (ch == '1') 
    {

    } 
   
  }
}
