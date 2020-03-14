/* 
 *  Muthanna Alwahash 2020
 *
 */

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
  _rstPin     = rstPIN;     // Reset Pin
  _sdioPin    = sdioPin;    // I2C Data IO Pin
  _sclkPin    = sclkPin;    // I2C Clock Pin
  _stcIntPin  = stcIntPin;  // Seek/Tune Complete Pin

}
//-----------------------------------------------------------------------------------------------------------------------------------
// Read the entire register set (0x00 - 0x0F) to Shadow
// Reading is in following register address sequence 0A,0B,0C,0D,0E,0F,00,01,02,03,04,05,06,07,08,09 = 16 Words = 32 bytes.
//-----------------------------------------------------------------------------------------------------------------------------------
void	Si4703::getShadow()
{
  Wire.requestFrom(I2C_ADDR, 32); 
  for(int i = 0 ; i<16; i++) {
    shadow.word[i] = (Wire.read()<<8) | Wire.read();
  }
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Write the current 9 control registers (0x02 to 0x07) to the Si4703
// The Si4703 assumes you are writing to 0x02 first, then increments
//-----------------------------------------------------------------------------------------------------------------------------------
byte 	Si4703::putShadow()
{
  Wire.beginTransmission(I2C_ADDR);
  for(int i = 8 ; i<14; i++) {              // i=8-13 >> Reg=0x02-0x07
    Wire.write(shadow.word[i] >> 8);        // Upper byte
    Wire.write(shadow.word[i] & 0x00FF);    // Lower byte
  }
  return Wire.endTransmission();            // End this transmission
}
//-----------------------------------------------------------------------------------------------------------------------------------
// To get the Si4703 in to 2-wire mode, SEN needs to be high and SDIO needs to be low after a reset
// The breakout board has SEN pulled high, but also has SDIO pulled high. Therefore, after a normal power up
// The Si4703 will be in an unknown state. RST must be controlled
//-----------------------------------------------------------------------------------------------------------------------------------
void Si4703::si4703_init() 
{
  // Set IO pins directions
  pinMode(_rstPin , OUTPUT);        // Reset pin
  pinMode(_sdioPin  , OUTPUT);      // I2C data IO pin
  pinMode(_stcIntPin, OUTPUT);	    // STC (search/tune complete) interrupt pin

  // Set communcation mode to 2-Wire
  digitalWrite(_sdioPin  ,LOW);     // A low SDIO indicates a 2-wire interface
  digitalWrite(_rstPin   ,LOW);     // Put Si4703 into reset
  digitalWrite(_stcIntPin,HIGH);    // STC goes low on interrupt
  delay(1);                         // Some delays while we allow pins to settle
  digitalWrite(_rstPin   ,HIGH);    // Bring Si4703 out of reset with SDIO set to low and SEN pulled high with on-board resistor
  delay(1);                         // Allow Si4703 to come out of reset
  Wire.begin();                     // Now that the unit is reset and I2C inteface mode, we need to begin I2C

  // Enable Oscillator
  getShadow();                      // Read the current register set
  shadow.reg.TEST1.bits.XOSCEN = 1; // Enable the oscillator
  putShadow();                      // Write to registers
  delay(500);                       // Wait for oscillator to settle

  // Start Configuration
  getShadow();                                      // Read the current register set

  // PowerOn Configuration
  shadow.reg.POWERCFG.bits.ENABLE   = 1;            // Powerup Enable
  shadow.reg.POWERCFG.bits.DISABLE  = 0;            // Powerup Disable
  shadow.reg.POWERCFG.bits.SEEK     = 0;            // Disable Seek
  shadow.reg.POWERCFG.bits.SEEKUP   = 1;            // Seek direction = UP
  shadow.reg.POWERCFG.bits.SKMODE   = 1;            // Seek mode = Wrap
  shadow.reg.POWERCFG.bits.RDSM     = 0;            // RDS Mode = Standard
  shadow.reg.POWERCFG.bits.MONO     = 1;            // Disable MONO Mode
  shadow.reg.POWERCFG.bits.DSMUTE   = 1;            // Disable Softmute
  shadow.reg.POWERCFG.bits.DMUTE    = 1;            // Disable Mute

  // System Configuration 1
  shadow.reg.SYSCONFIG1.bits.RDSIEN = 0;            // Disable RDS Interrupt
  shadow.reg.SYSCONFIG1.bits.STCIEN = 0;            // Disable Seek/Tune Complete Interrupt
  shadow.reg.SYSCONFIG1.bits.RDS    = 1;            // Enable RDS
  shadow.reg.SYSCONFIG1.bits.DE     = 0;            // De-emphasis=75 μs. Used in USA (default)
  shadow.reg.SYSCONFIG1.bits.AGCD   = 0;            // AGC enable
  shadow.reg.SYSCONFIG1.bits.BLNDADJ= 0x00;         // Stereo/Mono Blend Level Adjustment 31–49 RSSI dBμV (default)
  shadow.reg.SYSCONFIG1.bits.GPIO1  = GPIO_Z;       // GPIO1 = High impedance (default)
  shadow.reg.SYSCONFIG1.bits.GPIO2  = GPIO_Z;       // GPIO2 = High impedance (default)
  shadow.reg.SYSCONFIG1.bits.GPIO3  = GPIO_Z;       // GPIO3 = High impedance (default)

  // System Configuration 2
  shadow.reg.SYSCONFIG2.bits.VOLUME = 0;            // Set volume to 0
  shadow.reg.SYSCONFIG2.bits.SPACE  = SPACE_100KHz; // Select Channel Spacing Type
  shadow.reg.SYSCONFIG2.bits.BAND   = BAND_US_EU;   // 87.5–108 MHz (USA, Europe) (Default)
  shadow.reg.SYSCONFIG2.bits.SEEKTH = 0;            // 0x00 = min RSSI (default)
  
  // System Configuration 3
  shadow.reg.SYSCONFIG3.bits.SKCNT  = SKCNT_DIS;    // disabled (default)
  shadow.reg.SYSCONFIG3.bits.SKSNR  = SKSNR_DIS;    // disabled (default)
  shadow.reg.SYSCONFIG3.bits.VOLEXT = 0;            // disabled (default)
  shadow.reg.SYSCONFIG3.bits.SMUTEA = SMA_16dB;     // Softmute Attenuation 16dB (default)
  shadow.reg.SYSCONFIG3.bits.SMUTER = SMRR_Fastest; // Softmute Attack/Recover Rate = Fastest (default)

  putShadow();                                      // Write to registers
  delay(110);                                       // wait for max power up time
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Power Up Device
//-----------------------------------------------------------------------------------------------------------------------------------
void Si4703::powerUp()
{
    si4703_init();
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Power Down
//-----------------------------------------------------------------------------------------------------------------------------------
void Si4703::powerDown()
{
    
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
// Reads the current channel from READCHAN
// Returns a number like 974 for 97.4MHz
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::getChannel()
{
  getShadow();                                // Read the current register set
  
  // Freq (MHz) = Spacing (MHz) * Channel + Bottom of Band (MHz).
  return (bandSpacing * shadow.reg.READCHAN.bits.READCHAN + bandStart);  
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Sets Channel frequency
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::setChannel(int freq)
{

  getShadow();                              // Read the current register set
  if (freq > bandEnd)    freq = bandEnd;    // check upper limit
  if (freq < bandStart)  freq = bandStart;  // check lower limit

  // Freq (MHz) = Spacing (MHz) * Channel + Bottom of Band (MHz).
  shadow.reg.CHANNEL.bits.CHAN  = (freq - bandStart) / bandSpacing;
  shadow.reg.CHANNEL.bits.TUNE  = 1;        // Set the TUNE bit to start
  putShadow();                              // Write to registers

  while(_stcIntPin == 1) {}	//Wait for interrupt indicating STC (Seek/Tune Complete)

  getShadow();                              // Read the current register set
  shadow.reg.CHANNEL.bits.TUNE  =0;         // Clear Tune bit
  putShadow();                              // Write to registers

  //Wait for the si4703 to clear the STC
  while(1) {
    getShadow();                                  // Read the current register set
    if(shadow.reg.STATUSRSSI.bits.STC== 0) break; //Tuning complete!
  }

  return getChannel();
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Increment frequency one band step
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::incChannel(void)
{
  setChannel(getChannel()+bandSpacing); // Increment frequency one band step
  return getChannel();
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Decrement frequency one band step
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::decChannel(void)
{
  setChannel(getChannel()-bandSpacing); // Decrement frequency one band step
  return getChannel();
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Set Seek Mode
//-----------------------------------------------------------------------------------------------------------------------------------
void Si4703::setSeekMode()
{
  getShadow();                                      // Read the current register set
  shadow.reg.POWERCFG.bits.SKMODE   = 1;            // Seek mode = Wrap
  shadow.reg.SYSCONFIG2.bits.SEEKTH = 0;            // 0x00 = min RSSI (default)
  shadow.reg.SYSCONFIG3.bits.SKCNT  = SKCNT_DIS;    // disabled (default)
  shadow.reg.SYSCONFIG3.bits.SKSNR  = SKSNR_DIS;    // disabled (default)
  putShadow();                                      // Write to registers

}
//-----------------------------------------------------------------------------------------------------------------------------------
// Seeks the next available station
// Returns freq if seek succeeded
// Returns zero if seek failed
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::seek(byte seekDirection){

  getShadow();                                    // Read the current register set
  shadow.reg.POWERCFG.bits.SEEKUP=seekDirection; // Seek direction = UP/Down
  shadow.reg.POWERCFG.bits.SEEK     =1;           // Start seek
  putShadow();                                    // Write to registers

  while(_stcIntPin == 1) {}                       // Wait for interrupt indicating STC (Seek/Tune complete)
  
  getShadow();                                    // Read the current register set
  shadow.reg.CHANNEL.bits.TUNE  =0;               // Clear Tune bit
  putShadow();                                    // Write to registers
  
  while(1) {                                      // Wait for the si4703 to clear the STC
    getShadow();                                  // Read the current register set
    if(shadow.reg.STATUSRSSI.bits.STC== 0) break; // Tuning complete!
  }

  if(shadow.reg.STATUSRSSI.bits.SFBL)  return(0); // SFBL is indicating we hit a band limit or failed to find a station
  return getChannel();                            // return new frequency
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

}

//-----------------------------------------------------------------------------------------------------------------------------------
// Writes GPIO1-GPIO3
//-----------------------------------------------------------------------------------------------------------------------------------
	void	Si4703::writeGPIO(int GPIO, int val)
{
  getShadow();                                // Read the current register set

  switch (GPIO)
  {
    case GPIO1:
      shadow.reg.SYSCONFIG1.bits.GPIO1 = val;
      break;

    case GPIO2:
      shadow.reg.SYSCONFIG1.bits.GPIO2 = val;
      break;

    case GPIO3:
      shadow.reg.SYSCONFIG1.bits.GPIO3 = val;
      break;

    default:
      break;
  }
  
  putShadow();                                // Write to registers
}

//-----------------------------------------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------------------------------------
	int 	Si4703::getDeviceID()
  {
  }

//-----------------------------------------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------------------------------------
	int		Si4703::getChipID()
  {
  }