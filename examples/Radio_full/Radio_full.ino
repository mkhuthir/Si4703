/*  
 *   Terminal Controlled Radio using Si7403 Lib
 *   Muthanna Alwahash 2020
 *   
 */

//-------------------------------------------------------------------------------------------------------------
// Required Libraries
//-------------------------------------------------------------------------------------------------------------
#include <Si4703.h> // library to control Silicon Labs' Si4703 FM Radio Receiver.
#include <Wire.h>   // Used for I2C interface.
#include <EEPROM.h> // To save configuration parameters such as channel and volume.

//-------------------------------------------------------------------------------------------------------------
// Defines
//-------------------------------------------------------------------------------------------------------------

// EEPROM Usage Map
#define eeprom_chn_msb  1
#define eeprom_chn_lsb  2
#define eeprom_vol      3

// Used Pins
#define LED1        5       // LED1 pin
#define rotaryPinA  2       // encoder pin A
#define rotaryPinB  3       // encoder pin B. Note that rotaryPinC is connected to GND

// Direction
#define UP    true
#define DOWN  false

//-------------------------------------------------------------------------------------------------------------
// Variables
//-------------------------------------------------------------------------------------------------------------

// Favourate Channels fav_0..fav_9 (kHz)
int       fav_0 = 8760;   
int       fav_1 = 8820;   
int       fav_2 = 9140;  
int       fav_3 = 9220;   
int       fav_4 = 9390;  
int       fav_5 = 9440;   
int       fav_6 = 9500;  
int       fav_7 = 9760;   
int       fav_8 = 10480;  
int       fav_9 = 10740;   

//-------------------------------------------------------------------------------------------------------------
// Volatile variables for use in Rotary Encoder Interrupt Routine
//-------------------------------------------------------------------------------------------------------------
volatile int      rotaryLast      = 0b00;
volatile boolean  rotaryDirection = UP;
volatile boolean  rotaryUpdated   = false;

//-------------------------------------------------------------------------------------------------------------
// create radio instance using default settings
//-------------------------------------------------------------------------------------------------------------
Si4703 radio;

//-------------------------------------------------------------------------------------------------------------
// Arduino initial Setup
//-------------------------------------------------------------------------------------------------------------
void setup()
{
 
  Serial.begin(115200);       // start serial

  pinMode(LED1, OUTPUT);      // LED1 pin is output
  digitalWrite(LED1, LOW);    // turn LED1 OFF

  radio.start();              // start radio device
  read_EEPROM();              // load saved settings

  // Enable rotary encoder
  pinMode(rotaryPinA, INPUT_PULLUP);         // pin is input and pulled high
  pinMode(rotaryPinB, INPUT_PULLUP);         // pin is input and pulled high
  attachInterrupt(0, updateRotary, CHANGE);  // call updateEncoder() when any high/low changed seen on interrupt 0 (pin 2)
  attachInterrupt(1, updateRotary, CHANGE);  // call updateEncoder() when any high/low changed seen on interrupt 1 (pin 3)

  // Show ready status
  digitalWrite(LED1, HIGH);           // turn LED1 ON
  radio.writeGPIO(GPIO1, GPIO_High);  // turn LED2 ON

  // Display info
  printWelcome();
  printHelp();
  printCurrentSettings();
  
}
//-------------------------------------------------------------------------------------------------------------
// Arduino main loop
//-------------------------------------------------------------------------------------------------------------
void loop()
{

  if (rotaryUpdated)      updateChannel();  // Interrupt tells us to update the station when updateStation=True
  if (Serial.available()) processCommand(); // Radio control from serial interface

}
//-------------------------------------------------------------------------------------------------------------
// Write current settings to EEPROM
//-------------------------------------------------------------------------------------------------------------
void write_EEPROM()
{
  // Save current channel value
  int chan = radio.getChannel();      // get the current channel
  int msb = chan >> 8;                // move channel over 8 spots to grab MSB
  int lsb = chan & 0x00FF;            // clear the MSB, leaving only the LSB
  EEPROM.write(eeprom_chn_msb, msb);  // write each byte to a single 8-bit position
  EEPROM.write(eeprom_chn_lsb, lsb);

  // Save volume
  EEPROM.write(eeprom_vol, radio.getVolume());
  
}
//-------------------------------------------------------------------------------------------------------------
// Read settings from EEPROM
//-------------------------------------------------------------------------------------------------------------
void read_EEPROM()
{
  // Read channel value
  int MSB = EEPROM.read(eeprom_chn_msb);  // load the msb into one 8-bit register
  int LSB = EEPROM.read(eeprom_chn_lsb);  // load the lsb into one 8-bit register
  radio.setChannel((MSB << 8)|LSB);       // concatenate the lsb and msb
  
  // Read Volume
  radio.setVolume(EEPROM.read(eeprom_vol));
}
//-------------------------------------------------------------------------------------------------------------
// Interrupt handler that reads the encoder. It set the updateStation flag when a new indent is found 
//-------------------------------------------------------------------------------------------------------------
void updateRotary()
{
  int pinA = digitalRead(rotaryPinA); // read current value of Pin A
  int pinB = digitalRead(rotaryPinB); // read current value of Pin B

  int rotaryCurrent = (pinA << 1) |pinB;            // converting the 2 pins values to single number
  int pattern = (rotaryLast << 2) | rotaryCurrent;  // adding it to the previous encoded value

  if(pattern == 0b1101 || pattern == 0b0100 || pattern == 0b0010 || pattern == 0b1011)
    {
      rotaryDirection = DOWN;
      rotaryUpdated   = true;
    }
  
  if(pattern == 0b1110 || pattern == 0b0111 || pattern == 0b0001 || pattern == 0b1000)
    {
      rotaryDirection = UP;
      rotaryUpdated   = true;
    }
  
  rotaryLast = rotaryCurrent; //store current rotary AB values for next time

}
//-------------------------------------------------------------------------------------------------------------
// Update Channel Freq
//-------------------------------------------------------------------------------------------------------------
void updateChannel()
  {
    digitalWrite(LED1, LOW);           // turn LED1 OFF
    radio.writeGPIO(GPIO1, GPIO_Low);  // turn LED2 OFF

    if(rotaryDirection == UP)
      {
        radio.incChannel();
      }
    else if(rotaryDirection == DOWN)
      {
        radio.decChannel();
      }
    
    printCurrentSettings();     // Print channel info
    rotaryUpdated = false;      //Clear flag

    digitalWrite(LED1, HIGH);           // When done turn LED1 On
    radio.writeGPIO(GPIO1, GPIO_High);  // turn LED2 ON
  }
//-------------------------------------------------------------------------------------------------------------
// Display Welcome Message.
//-------------------------------------------------------------------------------------------------------------
void printWelcome()
{
  Serial.println("\nWelcome...");
  
  Serial.print("\nPart Number\t:0x");
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
//-------------------------------------------------------------------------------------------------------------
// Display current settings such as channel and volume.
//-------------------------------------------------------------------------------------------------------------
void printCurrentSettings()
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

//-------------------------------------------------------------------------------------------------------------
// Prints Favourite Stations List
//-------------------------------------------------------------------------------------------------------------
void printFavouriteList()
{
  Serial.println("List of Favourite Stations");
  
  Serial.print("0 - ");
  Serial.print(float(fav_0)/100,2);
  Serial.println(" MHz");

  Serial.print("1 - ");
  Serial.print(float(fav_1)/100,2);
  Serial.println(" MHz");

  Serial.print("2 - ");
  Serial.print(float(fav_2)/100,2);
  Serial.println(" MHz");

  Serial.print("3 - ");
  Serial.print(float(fav_3)/100,2);
  Serial.println(" MHz");

  Serial.print("4 - ");
  Serial.print(float(fav_4)/100,2);
  Serial.println(" MHz");

  Serial.print("5 - ");
  Serial.print(float(fav_5)/100,2);
  Serial.println(" MHz");

  Serial.print("6 - ");
  Serial.print(float(fav_6)/100,2);
  Serial.println(" MHz");

  Serial.print("7 - ");
  Serial.print(float(fav_7)/100,2);
  Serial.println(" MHz");

  Serial.print("8 - ");
  Serial.print(float(fav_8)/100,2);
  Serial.println(" MHz");

  Serial.print("9 - ");
  Serial.print(float(fav_9)/100,2);
  Serial.println(" MHz");

}

//-------------------------------------------------------------------------------------------------------------
// Display Help on commands.
//-------------------------------------------------------------------------------------------------------------
void printHelp()
{

  Serial.println("\n----------- Volume Control --------------");
  Serial.println("+ -     inc/Dec Volume (max 15)");
  Serial.println("e       Enable/Disable Volume 30dB Ext");
  Serial.println("m       Mute/Unmute volume");
  Serial.println("s       Set Mono/Sterio");
  Serial.println("----------- Tuneing -----------------------");
  Serial.println("u d     Frequency up / down");
  Serial.println("n l     Channel Seek next / last");
  Serial.println("0..9    Favourite stations");
  Serial.println("f       Prints Favourite stations list");
  Serial.println("----------- RDS & Info --------------------");
  Serial.println("r       Listen for RDS Data");
  Serial.println("i       Prints current settings");
  Serial.println("h       Prints this help");
  Serial.println("-------------------------------------------");
  Serial.println("Select a command:");
}

//-------------------------------------------------------------------------------------------------------------
// Process a command from serial terminal
//-------------------------------------------------------------------------------------------------------------
void processCommand()
{
  
  char ch = Serial.read();        // Read command
  Serial.print(" ");              // print single space after reading a command
  
  
  if (ch == '+')                  // Increment Volume (max 15)
    {
      radio.incVolume();
      printCurrentSettings();
    }
  else if (ch == '-')             // Decrement Volume (min 0)
    {
      radio.decVolume();
      printCurrentSettings();
    }
  
  else if (ch == 'e')             // Enable/Disable Volume 30dB Ext
    {
      radio.setVolExt(!radio.getVolExt()); // flip status
      printCurrentSettings();
    } 
  else if (ch == 'm')             // Mute/Unmute volume
    {
      radio.setMute(!radio.getMute()); // flip status
      printCurrentSettings();
    }
  else if (ch == 's')             // Set Mono/Sterio"
    {
      radio.setMono(!radio.getMono()); // flip status
      printCurrentSettings();
    }
  else if (ch == 'u')             // Tune Frequency up
    {
      digitalWrite(LED1, LOW);           // turn LED1 OFF
      radio.writeGPIO(GPIO1, GPIO_Low);  // turn LED2 OFF
      radio.incChannel();
      write_EEPROM();                    // Save channel to EEPROM
      printCurrentSettings();
      digitalWrite(LED1, HIGH);          // When done turn LED1 On
      radio.writeGPIO(GPIO1, GPIO_High); // turn LED2 ON
    } 
  else if (ch == 'd')             // Tune Frequency down
    {
      digitalWrite(LED1, LOW);           // turn LED1 OFF
      radio.writeGPIO(GPIO1, GPIO_Low);  // turn LED2 OFF
      radio.decChannel();
      write_EEPROM();                    // Save channel to EEPROM
      printCurrentSettings();
      digitalWrite(LED1, HIGH);          // When done turn LED1 On
      radio.writeGPIO(GPIO1, GPIO_High); // turn LED2 ON
    } 
  else if (ch == 'n')             // Channel Seek next
    {
      digitalWrite(LED1, LOW);           // turn LED1 OFF
      radio.writeGPIO(GPIO1, GPIO_Low);  // turn LED2 OFF
      if (!radio.seekUp()) 
        {
          Serial.println("| Error: Seek failure or band limit reached!!");
        }
      else
        {
          write_EEPROM();                    // Save channel to EEPROM
          printCurrentSettings();
        }
      digitalWrite(LED1, HIGH);          // When done turn LED1 On
      radio.writeGPIO(GPIO1, GPIO_High); // turn LED2 ON
    } 
  else if (ch == 'l')             // Channel Seek last
    {
      digitalWrite(LED1, LOW);           // turn LED1 OFF
      radio.writeGPIO(GPIO1, GPIO_Low);  // turn LED2 OFF
      if (!radio.seekDown())
        {
          Serial.println("| Error: Seek failure or band limit reached!!");
        }
      else
        {
          write_EEPROM();                    // Save channel to EEPROM
          printCurrentSettings();
        }
      digitalWrite(LED1, HIGH);          // When done turn LED1 On
      radio.writeGPIO(GPIO1, GPIO_High); // turn LED2 ON
    } 
  else if (ch == '0')             // Tune to favorite channel 0
    {
      radio.setChannel(fav_0);
      write_EEPROM();             // Save channel to EEPROM
      printCurrentSettings();
    }
  else if (ch == '1')             // Tune to favorite channel 1
    {
      radio.setChannel(fav_1);
      write_EEPROM();             // Save channel to EEPROM
      printCurrentSettings();
    }
  else if (ch == '2')             // Tune to favorite channel 2
    {
      radio.setChannel(fav_2);
      write_EEPROM();             // Save channel to EEPROM
      printCurrentSettings();
    }
  else if (ch == '3')             // Tune to favorite channel 3
    {
      radio.setChannel(fav_3);
      write_EEPROM();             // Save channel to EEPROM
      printCurrentSettings();
    }
  else if (ch == '4')             // Tune to favorite channel 4
    {
      radio.setChannel(fav_4);
      write_EEPROM();             // Save channel to EEPROM
      printCurrentSettings();
    }
  else if (ch == '5')             // Tune to favorite channel 5
    {
      radio.setChannel(fav_5);
      write_EEPROM();             // Save channel to EEPROM
      printCurrentSettings();
    }
  else if (ch == '6')             // Tune to favorite channel 6
    {
      radio.setChannel(fav_6);
      write_EEPROM();             // Save channel to EEPROM
      printCurrentSettings();
    }
  else if (ch == '7')             // Tune to favorite channel 7
    {
      radio.setChannel(fav_7);
      write_EEPROM();             // Save channel to EEPROM
      printCurrentSettings();
    }
  else if (ch == '8')             // Tune to favorite channel 8
    {
      radio.setChannel(fav_8);
      write_EEPROM();             // Save channel to EEPROM
      printCurrentSettings();
    }
  else if (ch == '9')             // Tune to favorite channel 9
    {
      radio.setChannel(fav_9);
      write_EEPROM();             // Save channel to EEPROM
      printCurrentSettings();
    }
  else if (ch == 'r')             // Listen for RDS Data
    {
      // TODO:
    }
  else if (ch == 'i')             // Print current settings
    {
      printCurrentSettings();
    }
  else if (ch == 'f')             // List favorite channels
    {
      printFavouriteList();
    }
  else if (ch == 'h')             // Print help
    {
      printHelp();
    }
  else                            // Unknown commands
    {
      Serial.print("Unknown command:'");
      Serial.print(ch);
      Serial.print("'");
      Serial.println(" send 'h' for help.");
    }
}
