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
//-------------------------------------------------------------------------------------------------------------
// Global Constants (defines): these quantities don't change
//-------------------------------------------------------------------------------------------------------------
const int RST         = 4;  // radio reset pin
const int SDIO        = A4; // radio data pin
const int SCLK        = A5; // radio clock pin
const int STC         = 6;  // radio interrupt pin
const int LED1        = 5;  // LED1 pin
const int rotaryPinA  = 2;  // encoder pin A
const int rotaryPinB  = 3;  // encoder pin B. Note that rotaryPinC is connected to GND
const boolean UP      = true;
const boolean DOWN    = false;
//-------------------------------------------------------------------------------------------------------------
// Variables
//-------------------------------------------------------------------------------------------------------------

// Settings
int       mute    =0;   // mute value volume=0

// Favourate Channels 0..9
int       fav_0         =876;   
int       fav_1         =882;   
int       fav_2         =914;  
int       fav_3         =922;   
int       fav_4         =939;  
int       fav_5         =944;   
int       fav_6         =950;  
int       fav_7         =976;   
int       fav_8         =1048;  
int       fav_9         =1074;   


char      rdsBuffer[10];        // Buffer to store RDS/RBDS text

//-------------------------------------------------------------------------------------------------------------
// Volatile variables for use in Rotary Encoder Interrupt Routine
//-------------------------------------------------------------------------------------------------------------
volatile int      rotaryLast      = 0b00;
volatile boolean  rotaryDirection = UP;
volatile boolean  rotaryUpdated   = false;

//-------------------------------------------------------------------------------------------------------------
// create radio instance
//-------------------------------------------------------------------------------------------------------------
Si4703 radio(RST, SDIO, SCLK, STC);

//-------------------------------------------------------------------------------------------------------------
// Arduino initial Setup
//-------------------------------------------------------------------------------------------------------------
void setup()
{
 
  Serial.begin(115200);       // start serial

  pinMode(LED1, OUTPUT);      // LED1 pin is output
  digitalWrite(LED1, LOW);    // turn LED1 OFF

  radio.powerUp();            // turns the module on
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
  printCurrentSettings();
  printHelp();
  
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
  int MSB = EEPROM.read(eeprom_chn_msb); // load the msb into one 8-bit register
  int LSB = EEPROM.read(eeprom_chn_lsb); // load the lsb into one 8-bit register
  radio.setChannel((MSB << 8)|LSB);              // concatenate the lsb and msb
  
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
}

//-------------------------------------------------------------------------------------------------------------
// Display current settings such as channel and volume.
//-------------------------------------------------------------------------------------------------------------
void printCurrentSettings()
{
   Serial.print("Ch:");
   Serial.print(float(radio.getChannel())/10,2);
   Serial.print(" MHz sVOL:");
   Serial.println(radio.getVolume());
}

//-------------------------------------------------------------------------------------------------------------
// Display Help on commands.
//-------------------------------------------------------------------------------------------------------------
void printHelp()
{
  Serial.println("0..9    Favourite stations");
  Serial.println("+ -     Volume (max 15)");
  Serial.println("u d     Frequency up / down");
  Serial.println("n l     Channel Seek next / last");
  Serial.println("r       Listen for RDS Data (15 sec timeout)");
  Serial.println("i       Prints current settings");
  Serial.println("f       Prints Favourite stations list");
  Serial.println("m       Mute/Unmute sound");
  Serial.println("h       Prints this help");
  Serial.println("Select a command:");
}

//-------------------------------------------------------------------------------------------------------------
// Prints Favourite Stations List
//-------------------------------------------------------------------------------------------------------------
void printFavouriteList()
{
  Serial.println("List of Favourite Stations");
  
  Serial.print("0 - ");
  Serial.print(float(fav_0)/10,1);
  Serial.println(" MHz");

  Serial.print("1 - ");
  Serial.print(float(fav_1)/10,1);
  Serial.println(" MHz");

  Serial.print("2 - ");
  Serial.print(float(fav_2)/10,1);
  Serial.println(" MHz");

  Serial.print("3 - ");
  Serial.print(float(fav_3)/10,1);
  Serial.println(" MHz");

  Serial.print("4 - ");
  Serial.print(float(fav_4)/10,1);
  Serial.println(" MHz");

  Serial.print("5 - ");
  Serial.print(float(fav_5)/10,1);
  Serial.println(" MHz");

  Serial.print("6 - ");
  Serial.print(float(fav_6)/10,1);
  Serial.println(" MHz");

  Serial.print("7 - ");
  Serial.print(float(fav_7)/10,1);
  Serial.println(" MHz");

  Serial.print("8 - ");
  Serial.print(float(fav_8)/10,1);
  Serial.println(" MHz");

  Serial.print("9 - ");
  Serial.print(float(fav_9)/10,1);
  Serial.println(" MHz");

}
//-------------------------------------------------------------------------------------------------------------
// Process a command from serial terminal
//-------------------------------------------------------------------------------------------------------------
void processCommand()
{
  
  char ch = Serial.read();
  Serial.print(" ");
  
  if (ch == 'n') 
  {
    digitalWrite(LED1, LOW);           // turn LED1 OFF
    radio.writeGPIO(GPIO1, GPIO_Low);  // turn LED2 OFF
    radio.seekUp();
    write_EEPROM();                    // Save channel to EEPROM
    printCurrentSettings();
    digitalWrite(LED1, HIGH);          // When done turn LED1 On
    radio.writeGPIO(GPIO1, GPIO_High); // turn LED2 ON
  } 
  else if (ch == 'l') 
  {
    digitalWrite(LED1, LOW);           // turn LED1 OFF
    radio.writeGPIO(GPIO1, GPIO_Low);  // turn LED2 OFF
    radio.seekDown();
    write_EEPROM();                    // Save channel to EEPROM
    printCurrentSettings();
    digitalWrite(LED1, HIGH);          // When done turn LED1 On
    radio.writeGPIO(GPIO1, GPIO_High); // turn LED2 ON
  } 
  else if (ch == 'u') 
  {
    digitalWrite(LED1, LOW);           // turn LED1 OFF
    radio.writeGPIO(GPIO1, GPIO_Low);  // turn LED2 OFF
    radio.incChannel();
    write_EEPROM();                    // Save channel to EEPROM
    printCurrentSettings();
    digitalWrite(LED1, HIGH);          // When done turn LED1 On
    radio.writeGPIO(GPIO1, GPIO_High); // turn LED2 ON
  } 
  else if (ch == 'd') 
  {
    digitalWrite(LED1, LOW);           // turn LED1 OFF
    radio.writeGPIO(GPIO1, GPIO_Low);  // turn LED2 OFF
    radio.decChannel();
    write_EEPROM();                    // Save channel to EEPROM
    printCurrentSettings();
    digitalWrite(LED1, HIGH);          // When done turn LED1 On
    radio.writeGPIO(GPIO1, GPIO_High); // turn LED2 ON
  } 
  else if (ch == '+') 
  {
    radio.incVolume();
    printCurrentSettings();
  } 
  else if (ch == '-') 
  {
    radio.decVolume();
    printCurrentSettings();
  } 
  else if (ch == '0')
  {
    radio.setChannel(fav_0);
    write_EEPROM();             // Save channel to EEPROM
    printCurrentSettings();
  }
  else if (ch == '1')
  {
    radio.setChannel(fav_1);
    write_EEPROM();             // Save channel to EEPROM
    printCurrentSettings();
  }
  else if (ch == '2')
  {
    radio.setChannel(fav_2);
    write_EEPROM();             // Save channel to EEPROM
    printCurrentSettings();
  }
  else if (ch == '3')
  {
    radio.setChannel(fav_3);
    write_EEPROM();             // Save channel to EEPROM
    printCurrentSettings();
  }
  else if (ch == '4')
  {
    radio.setChannel(fav_4);
    write_EEPROM();             // Save channel to EEPROM
    printCurrentSettings();
  }
  else if (ch == '5')
  {
    radio.setChannel(fav_5);
    write_EEPROM();             // Save channel to EEPROM
    printCurrentSettings();
  }
  else if (ch == '6')
  {
    radio.setChannel(fav_6);
    write_EEPROM();             // Save channel to EEPROM
    printCurrentSettings();
  }
  else if (ch == '7')
  {
    radio.setChannel(fav_7);
    write_EEPROM();             // Save channel to EEPROM
    printCurrentSettings();
  }
  else if (ch == '8')
  {
    radio.setChannel(fav_8);
    write_EEPROM();             // Save channel to EEPROM
    printCurrentSettings();
  }
  else if (ch == '9')
  {
    radio.setChannel(fav_9);
    write_EEPROM();             // Save channel to EEPROM
    printCurrentSettings();
  }
  else if (ch == 'r')
  {
    Serial.println("RDS listening");
    radio.readRDS(rdsBuffer, 15000);
    Serial.print("RDS heard:");
    Serial.println(rdsBuffer);      
  }
  else if (ch == 'i')
  {
    printCurrentSettings();
  }
    else if (ch == 'f')
  {
    printFavouriteList();
  }
  else if (ch == 'm')
  {
    int temp  = radio.getVolume();  // Save the current volume in temp
    radio.setVolume(mute);          // Set Volume
    mute      = temp;               // Save last volume in mute
    printCurrentSettings();
  }
  else if (ch == 'h')
  {
    printHelp();
  }
  else
  {
    Serial.print("Unknown command:'");
    Serial.print(ch);
    Serial.print("'");
    Serial.println(" send 'h' for help.");
  }
}
