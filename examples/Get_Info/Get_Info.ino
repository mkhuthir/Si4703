#include <Si4703.h>
#include <Wire.h>

Si4703 radio;             // using default values for all settings

void setup()
{
  Serial.begin(115200);   // start serial
  radio.start();          // Power Up Device
  radio.setChannel(8760); // Set frequency
  radio.setVolume(1);     // Set volume

  
  Serial.print("Part Number\t:0x");
  Serial.println(radio.getPN(), HEX);

  Serial.print("Manufacturer ID\t:0x");
  Serial.println(radio.getMFGID(),HEX);

  Serial.print("Device\t\t:0x");
  Serial.println(radio.getDEV(),HEX);

  Serial.print("Chip Version\t:0x");
  Serial.println(radio.getREV(),HEX);

  Serial.print("Firmware Version:0x");
  Serial.println(radio.getFIRMWARE(),HEX);

  Serial.print("\nBand Start\t:");
  Serial.print(float(radio.getBandStart())/100,2);
  Serial.println(" MHz");

  Serial.print("Band End\t:");
  Serial.print(float(radio.getBandEnd())/100,2);
  Serial.println(" MHz");

  Serial.print("Band Space\t:");
  Serial.print(float(radio.getBandSpace())/100,2);
  Serial.println(" MHz");
}

void loop()
{
}
