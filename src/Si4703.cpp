#include "Arduino.h"
#include "Si4703.h"
#include "Wire.h"

//-----------------------------------------------------------------------------------------------------------------------------------
// Si4703 Class Initialization
//-----------------------------------------------------------------------------------------------------------------------------------
Si4703::Si4703( int rstPIN, 
                int sdioPin,
                int sclkPin,
                int stcIntPin
              )
{
  _rstPin     = rstPIN;   // Reset Pin
  _sdioPin    = sdioPin;    // I2C Data IO Pin
  _sclkPin    = sclkPin;    // I2C Clock Pin
  _stcIntPin  = stcIntPin;  // Seek/Tune Complete Pin

}

//-----------------------------------------------------------------------------------------------------------------------------------
// Read the entire register set (0x00 - 0x0F) to Shadow
// Reading is in following register address sequence 0A,0B,0C,0D,0E,0F,00,01,02,03,04,05,06,07,08,09 = 16 Words = 32 bytes.
//-----------------------------------------------------------------------------------------------------------------------------------
void	Si4703::getShadow(){

  Wire.requestFrom(I2C_ADDR, 32); 
  for(int i = 0 ; i<16; i++) {
    shadow.word[i] = (Wire.read()<<8) | Wire.read();
  }

}

//-----------------------------------------------------------------------------------------------------------------------------------
// Write the current 9 control registers (0x02 to 0x07) to the Si4703
// The Si4703 assumes you are writing to 0x02 first, then increments
//-----------------------------------------------------------------------------------------------------------------------------------
byte 	Si4703::putShadow(){

  Wire.beginTransmission(I2C_ADDR);
  for(int i = 8 ; i<14; i++) {              // i=8-13 >> Reg=0x02-0x07
    Wire.write(shadow.word[i] >> 8);        // Upper byte
    Wire.write(shadow.word[i] & 0x00FF);    // Lower byte
  }
  return Wire.endTransmission();            // End this transmission

}

//-----------------------------------------------------------------------------------------------------------------------------------
void Si4703::readRegisters(){
  
  Wire.requestFrom(I2C_ADDR, 32);
  for(int x = 0x0A ; ; x++) {                       
      if(x == 0x10) x = 0;
      si4703_registers[x] = (Wire.read()<<8) | Wire.read();
      if(x == 0x09) break; 
    }
}
//-----------------------------------------------------------------------------------------------------------------------------------
byte Si4703::updateRegisters() {

  Wire.beginTransmission(I2C_ADDR);
  for(int regSpot = 0x02 ; regSpot < 0x08 ; regSpot++) {
    byte high_byte = si4703_registers[regSpot] >> 8;
    byte low_byte = si4703_registers[regSpot] & 0x00FF;

    Wire.write(high_byte);  //Upper 8 bits
    Wire.write(low_byte);   //Lower 8 bits
  }

  return Wire.endTransmission(); // End this transmission
  
}

//-----------------------------------------------------------------------------------------------------------------------------------
// To get the Si4703 in to 2-wire mode, SEN needs to be high and SDIO needs to be low after a reset
// The breakout board has SEN pulled high, but also has SDIO pulled high. Therefore, after a normal power up
// The Si4703 will be in an unknown state. RST must be controlled
//-----------------------------------------------------------------------------------------------------------------------------------
void Si4703::si4703_init() 
{
  // Set IO pins directions
  pinMode(_rstPin , OUTPUT);      // Reset pin
  pinMode(_sdioPin  , OUTPUT);      // I2C data IO pin
  pinMode(_stcIntPin, OUTPUT);	    // STC (search/tune complete) interrupt pin

  // Set communcation mode to 2-Wire
  digitalWrite(_sdioPin   , LOW);   // A low SDIO indicates a 2-wire interface
  digitalWrite(_rstPin  , LOW);   // Put Si4703 into reset
  digitalWrite(_stcIntPin , HIGH);  // STC goes low on interrupt
  delay(1);                         // Some delays while we allow pins to settle
  digitalWrite(_rstPin  , HIGH);  // Bring Si4703 out of reset with SDIO set to low and SEN pulled high with on-board resistor
  delay(1);                         // Allow Si4703 to come out of reset

  // Enable Oscillator
  Wire.begin();                     // Now that the unit is reset and I2C inteface mode, we need to begin I2C
  getShadow();                      // Read the current register set
  shadow.reg.TEST1.bits.XOSCEN = 1; // Enable the oscillator
  putShadow();                      // Write to registers
  delay(500);                       // Wait for oscillator to settle

  // PowerOn Configuration
  getShadow();                                      // Read the current register set
  shadow.reg.POWERCFG.bits.ENABLE   = 1;            // Enable chip
  shadow.reg.POWERCFG.bits.MONO     = 1;            // Stereo Mode
  shadow.reg.POWERCFG.bits.DSMUTE   = 1;            // Disable Softmute
  shadow.reg.POWERCFG.bits.DMUTE    = 1;            // Disable Dynamic Mute

  // System Configuration 1
  shadow.reg.SYSCONFIG1.bits.STCIEN = 1;            //  Enable Seek/Tune Complete Interrupt
  shadow.reg.SYSCONFIG1.bits.RDS    = 1;            //  Enable RDS
  shadow.reg.SYSCONFIG1.bits.DE     = 1;            //  

  // System Configuration 2
  shadow.reg.SYSCONFIG2.bits.SPACE  = SPACE_100KHz; // Select Channel Spacing Type
  shadow.reg.SYSCONFIG2.bits.VOLUME = 0;            // Set volume to 0
  putShadow();                                      // Write to registers
  delay(110);                                       // wait for max power up time
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Power On 
//-----------------------------------------------------------------------------------------------------------------------------------
void Si4703::powerOn()
{
    si4703_init();
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Set Volume
//-----------------------------------------------------------------------------------------------------------------------------------
void Si4703::setVolume(int volume)
{
  getShadow();                                // Read the current register set
  if (volume < 0 ) volume = 0;                // Accepted Volume value 0-15
  if (volume > 15) volume = 15;               // Accepted Volume value 0-15
  shadow.reg.SYSCONFIG2.bits.VOLUME = volume; // Set volume to 0
  putShadow();                                // Write to registers
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Set Channel
//-----------------------------------------------------------------------------------------------------------------------------------
void Si4703::setChannel(int channel)
{
  // Europe Freq MHz = 0.100 MHz * Channel + 87.5 MHz
  // US     Freq MHz = 0.200 MHz * Channel + 87.5 MHz
  
  int newChannel = channel - 875;
      
  //These steps come from AN230 page 20 rev 0.5
  readRegisters();
  si4703_registers[CHANNEL] &= 0xFE00;      //Clear out the channel bits
  si4703_registers[CHANNEL] |= newChannel;  //Mask in the new channel
  si4703_registers[CHANNEL] |= (1<<TUNE);   //Set the TUNE bit to start
  updateRegisters();

  while(_stcIntPin == 1) {}	//Wait for interrupt indicating STC (Seek/Tune Complete)

  readRegisters();
  si4703_registers[CHANNEL] &= ~(1<<TUNE); //Clear the tune after a tune has completed
  updateRegisters();

  //Wait for the si4703 to clear the STC as well
  while(1) {
    readRegisters();
    if( (si4703_registers[STATUSRSSI] & (1<<STC)) == 0) break; //Tuning complete!
  }
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Reads the current channel from READCHAN
// Returns a number like 973 for 97.3MHz
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::getChannel() {
  readRegisters();

  // Europe Freq MHz = 0.100 MHz * Channel + 87.5 MHz
  // US     Freq MHz = 0.200 MHz * Channel + 87.5 MHz
  
  int channel = si4703_registers[READCHAN] & 0x03FF;  // Mask out everything but the lower 10 bits
      channel += 875;                                 //98 + 875 = 973

  return(channel);
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Seeks out the next available station
// Returns the freq if it made it
// Returns zero if failed
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::seek(byte seekDirection){
  readRegisters();
  // Set seek mode wrap bit
  si4703_registers[POWERCFG] |= (1<<SKMODE); //Allow wrap

  // si4703_registers[POWERCFG] &= ~(1<<SKMODE); //Disallow wrap - if you disallow wrap, you may want to tune to 87.5 first
  if(seekDirection == SEEK_DOWN) si4703_registers[POWERCFG] &= ~(1<<SEEKUP); //Seek down is the default upon reset
  else si4703_registers[POWERCFG] |= 1<<SEEKUP; //Set the bit to seek up

  si4703_registers[POWERCFG] |= (1<<SEEK); //Start seek
  updateRegisters(); // Seeking will now start

  while(_stcIntPin == 1) {} // Wait for interrupt indicating STC (Seek/Tune complete)

  readRegisters();
  int valueSFBL = si4703_registers[STATUSRSSI] & (1<<SFBL); //Store the value of SFBL
  si4703_registers[POWERCFG] &= ~(1<<SEEK); // Clear the seek bit after seek has completed
  updateRegisters();

  //Wait for the si4703 to clear the STC as well
  while(1) {
    readRegisters();
    if( (si4703_registers[STATUSRSSI] & (1<<STC)) == 0) break; // Tuning complete!
  }

  if(valueSFBL) { // The bit was set indicating we hit a band limit or failed to find a station
    return(0);
  }
return getChannel();
}

//----------------------------------------------------------------------------------------------------------------------------------
// Seek Up
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::seekUp()
{
	return seek(SEEK_UP);
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Seek Down
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::seekDown()
{
	return seek(SEEK_DOWN);
}



//-----------------------------------------------------------------------------------------------------------------------------------
// Read RDS
//-----------------------------------------------------------------------------------------------------------------------------------
void Si4703::readRDS(char* buffer, long timeout)
{ 
	long    endTime         = millis() + timeout;
  boolean completed[]     = {false, false, false, false};
  int     completedCount  = 0;

  while(completedCount < 4 && millis() < endTime) {
    
    readRegisters();
    
    if(si4703_registers[STATUSRSSI] & (1<<RDSR)){
      // ls 2 bits of B determine the 4 letter pairs
      // once we have a full set return
      // if you get nothing after 20 readings return with empty string
      uint16_t b = si4703_registers[RDSB];
      int index = b & 0x03;
      if (! completed[index] && b < 500)
      {
        completed[index] = true;
        completedCount   ++;
        char Dh = (si4703_registers[RDSD] & 0xFF00) >> 8;
        char Dl = (si4703_registers[RDSD] & 0x00FF);
        buffer[index * 2]     = Dh;
        buffer[index * 2 +1]  = Dl;
       
      }

      delay(40); //Wait for the RDS bit to clear
    }
    
    else {
      delay(30); //From AN230, using the polling method 40ms should be sufficient amount of time between checks
    }

  }

	if (millis() >= endTime) {
		buffer[0] ='\0';
		return;
	}

  buffer[8] = '\0';
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Writes GPIO1-3
//-----------------------------------------------------------------------------------------------------------------------------------
	void	Si4703::writeGPIO(int GPIO, int val)
{
  readRegisters();                          // Read the current register set

  if (val==GPIO_Z) {

    si4703_registers[SYSCONFIG1] &= ~GPIO;  // Clear GPIO bits (00)
  }

  else if (val==GPIO_I) {

    si4703_registers[SYSCONFIG1] &= ~GPIO;  // Clear GPIO bits
    GPIO &= 0b00010101;
    si4703_registers[SYSCONFIG1] |= GPIO;   // Set GPIO bits to (01)
  }

  else if (val==GPIO_Low) {

    si4703_registers[SYSCONFIG1] &= ~GPIO;  // Clear GPIO bits
    GPIO &= 0b00101010;
    si4703_registers[SYSCONFIG1] |= GPIO;   // Set GPIO bits to (10)
  }

  else if (val==GPIO_High) {

    si4703_registers[SYSCONFIG1] |= GPIO;   // Set GPIO bits (11)
  }

  else {
    
    Serial.println("Error undefined GPIO value!");
  }
  
  updateRegisters();
}

//-----------------------------------------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------------------------------------
	DEVICEID_t 	Si4703::getDeviceID()
  {
  }

//-----------------------------------------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------------------------------------
	int		Si4703::getChipID()
  {
  }