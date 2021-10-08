/* 
 *  Muthanna Alwahash 2020/21
 *
 */

#include "Arduino.h"
#include "Si4703.h"
#include "Wire.h"

//-----------------------------------------------------------------------------------------------------------------------------------
// Si4703 Class Initialization
//-----------------------------------------------------------------------------------------------------------------------------------
Si4703::Si4703( 
                // MCU Pins Selection
                int rstPin    = 4,            // Reset Pin
			          int sdioPin   = A4,           // I2C Data IO Pin
			          int sclkPin   = A5,           // I2C Clock Pin
			          int intPin    = 0,	          // Seek/Tune Complete and RDS interrupt Pin

                // Band Settings
			          int band      = BAND_US_EU,	  // Band Range
                int space     = SPACE_100KHz,	// Band Spacing
                int de        = DE_75us,		  // De-Emphasis
                
                // RDS Settings

                // Tune Settings

                // Seek Settings
			          int skmode    = SKMODE_STOP,	// Seek Mode
			          int seekth    = 24,	          // Seek Threshold
			          int skcnt 	  = SKSNR_MAX,    // Seek Clicks Number Threshold
			          int sksnr		  = SKCNT_MIN,    // Seek Signal/Noise Ratio
                int agcd	    = 0				      // AGC disable
              )
{
  // MCU Pins Selection
  _rstPin   = rstPin;   // Reset Pin
  _sdioPin  = sdioPin;  // I2C Data IO Pin
  _sclkPin  = sclkPin;  // I2C Clock Pin
  _intPin   = intPin;   // Seek/Tune Complete Pin

  // Band Settings
  _band     = band;	    // Band Range
  _space    = space;    // Band Spacing
  _de       =	de;	      // De-Emphasis

  // RDS Settings

  // Tune Settings

  // Seek Settings
  _skmode   =	skmode;   // Seek Mode Wrap/Stop
	_seekth   =	seekth;   // Seek Threshold
	_skcnt    =	skcnt;    // Seek Clicks Number Threshold
	_sksnr    =	sksnr;	  // Seek Signal/Noise Ratio
  _agcd     = agcd;     // AGC disable
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
// 3-Wire Control Interface (SCLK, SEN, SDIO)
//-----------------------------------------------------------------------------------------------------------------------------------

void	Si4703::bus3Wire(void)
{
  // TODO:
}			
//-----------------------------------------------------------------------------------------------------------------------------------
// 2-Wire Control Interface (SCLCK, SDIO)
//-----------------------------------------------------------------------------------------------------------------------------------
void	Si4703::bus2Wire(void)		
{
  // Set IO pins directions
  pinMode(_rstPin , OUTPUT);    // Reset pin
  pinMode(_sdioPin, OUTPUT);    // I2C data IO pin
  
  // Set communcation mode to 2-Wire
  digitalWrite(_rstPin ,LOW);   // Put Si4703 into reset
  digitalWrite(_sdioPin,LOW);   // A low SDIO indicates a 2-wire interface
  delay(1);                     // Delay to allow pins to settle
  digitalWrite(_rstPin ,HIGH);  // Bring Si4703 out of reset with SDIO set to low and SEN pulled high with on-board resistor
  delay(1);                     // Allow Si4703 to come out of reset
  Wire.begin();                 // Now that the unit is reset and I2C inteface mode, we need to begin I2C

}	
//-----------------------------------------------------------------------------------------------------------------------------------
// Power Up Device
//-----------------------------------------------------------------------------------------------------------------------------------
void Si4703::powerUp()
{
  // Enable Oscillator
  getShadow();                            // Read the current register set
  shadow.reg.TEST1.bits.XOSCEN = 1;       // Enable the oscillator
  putShadow();                            // Write to registers
  delay(500);                             // Wait for oscillator to settle

  // Enable Device
  getShadow();                            // Read the current register set
  shadow.reg.POWERCFG.bits.ENABLE   = 1;  // Powerup Enable=1
  shadow.reg.POWERCFG.bits.DISABLE  = 0;  // Powerup Disable=0
  shadow.reg.POWERCFG.bits.DMUTE    = 1;  // Disable Mute
  putShadow();                            // Write to registers
  delay(110);                             // wait for max power up time
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Power Down
//-----------------------------------------------------------------------------------------------------------------------------------
void Si4703::powerDown()
{
  getShadow();                                // Read the current register set
  shadow.reg.TEST1.bits.AHIZEN      = 1;      // LOUT/LOUT = High impedance

  shadow.reg.SYSCONFIG1.bits.GPIO1  = GPIO_Z; // GPIO1 = High impedance (default)
  shadow.reg.SYSCONFIG1.bits.GPIO2  = GPIO_Z; // GPIO2 = High impedance (default)
  shadow.reg.SYSCONFIG1.bits.GPIO3  = GPIO_Z; // GPIO3 = High impedance (default)

  shadow.reg.POWERCFG.bits.DMUTE    = 0;      // Disable Mute
  shadow.reg.POWERCFG.bits.ENABLE   = 1;      // PowerDown Enable=1
  shadow.reg.POWERCFG.bits.DISABLE  = 1;      // PowerDown Disable=1
  
  putShadow();                                // Write to registers
  delay(2);                                   // wait for max power down time
}
//-----------------------------------------------------------------------------------------------------------------------------------
// To get the Si4703 in to 2-wire mode, SEN needs to be high and SDIO needs to be low after a reset
// The breakout board has SEN pulled high, but also has SDIO pulled high. Therefore, after a normal power up
// The Si4703 will be in an unknown state. RST must be controlled
//-----------------------------------------------------------------------------------------------------------------------------------
void Si4703::start() 
{
  bus2Wire();   // 2-Wire Control Interface (SCLCK, SDIO)
  powerUp();    // Power Up device

  // Default Start Configuration
  getShadow();                            // Read the current register set

  // Select region band
  setRegion(_band,_space,_de);                      // Select region band limits
  shadow.reg.SYSCONFIG2.bits.SPACE  = _space;       // Select Channel Spacing Type
  shadow.reg.SYSCONFIG2.bits.BAND   = _band;        // Select Band frequency range
  shadow.reg.SYSCONFIG1.bits.DE     = _de;          // Select de-emphasis                          

  // Set Tune
  shadow.reg.SYSCONFIG1.bits.STCIEN = 0;            // Disable Seek/Tune Complete Interrupt

  // Set seek mode
  shadow.reg.POWERCFG.bits.SEEK     = 0;            // Disable Seek
  shadow.reg.POWERCFG.bits.SEEKUP   = 1;            // Seek direction = UP
  shadow.reg.POWERCFG.bits.SKMODE   = _skmode;      // Seek mode Wrap/Stop
  shadow.reg.SYSCONFIG2.bits.SEEKTH = _seekth;      // Seek Threshold
  shadow.reg.SYSCONFIG3.bits.SKCNT  = _skcnt;       // Seek Clicks Number Threshold
  shadow.reg.SYSCONFIG3.bits.SKSNR  = _sksnr;       // Seek Signal/Noise Ratio
  shadow.reg.SYSCONFIG1.bits.AGCD   = _agcd;        // AGC Disable

  // Set RDS mode
  shadow.reg.SYSCONFIG1.bits.RDSIEN = 0;            // Enable/Disable RDS Interrupt
  shadow.reg.POWERCFG.bits.RDSM     = 0;            // RDS Mode Standard/Verbose
  shadow.reg.SYSCONFIG1.bits.RDS    = 1;            // Enable/Disable RDS

  // Set Audio
  shadow.reg.TEST1.bits.AHIZEN      = 0;            // Enable Audio
  shadow.reg.POWERCFG.bits.MONO     = 0;            // Disable MONO Mode
  shadow.reg.SYSCONFIG1.bits.BLNDADJ= BLA_31_49;    // Stereo/Mono Blend Level Adjustment 31–49 RSSI dBμV (default)
  shadow.reg.SYSCONFIG2.bits.VOLUME = 0;            // Set volume to 0
  shadow.reg.SYSCONFIG3.bits.VOLEXT = 0;            // disabled (default)
  
  // Set Softmute mode
  shadow.reg.POWERCFG.bits.DSMUTE   = 1;            // Disable Softmute
  shadow.reg.SYSCONFIG3.bits.SMUTEA = SMA_16dB;     // Softmute Attenuation 16dB (default)
  shadow.reg.SYSCONFIG3.bits.SMUTER = SMRR_Fastest; // Softmute Attack/Recover Rate = Fastest (default)

  // Set GPIOs
  shadow.reg.SYSCONFIG1.bits.GPIO1  = GPIO_Z;       // GPIO1 = High impedance (default)
  shadow.reg.SYSCONFIG1.bits.GPIO2  = GPIO_Z;       // GPIO2 = High impedance (default)
  shadow.reg.SYSCONFIG1.bits.GPIO3  = GPIO_Z;       // GPIO3 = High impedance (default)

  putShadow();                                      // Write to registers
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Set FM Band Region limits and spacing
//-----------------------------------------------------------------------------------------------------------------------------------
void	Si4703::setRegion(int band,	  // Band Range
                        int space,	// Band Spacing
                        int de)		  // De-Emphasis
{
  switch (band)
  {
    case BAND_US_EU:      // 87.5–108 MHz (US / Europe, Default)
      _bandStart = 8750;  // Bottom of Band (kHz)
      _bandEnd		= 10800;	// Top of Band (kHz)
      break;
    
    case BAND_JPW:        // 76–108 MHz (Japan wide band)
      _bandStart = 7600;  // Bottom of Band (kHz)
      _bandEnd		= 10800;	// Top of Band (kHz)
      break;
    
    case BAND_JP:         // 76–90 MHz (Japan)
      _bandStart = 7600;	// Bottom of Band (kHz)
      _bandEnd		= 9000;	// Top of Band (kHz)
      break;

    default:
      break;
  }

  switch (space)
  {
    case SPACE_100KHz:    // 200 kHz (US / Australia, Default)
      _bandSpacing	= 10;	// Band Spacing (kHz)
      break;

    case SPACE_200KHz:    // 100 kHz (Europe / Japan)
      _bandSpacing	= 20;	// Band Spacing (kHz)
      break;

    case SPACE_50KHz:     // 50 kHz (Other)
      _bandSpacing	= 5;		// Band Spacing (kHz)
      break;
    
    default:
      break;
  }
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Set Mono
//-----------------------------------------------------------------------------------------------------------------------------------
void	Si4703::setMono(bool en)
{
  getShadow();                            // Read the current register set
  shadow.reg.POWERCFG.bits.MONO = en;     // 1 = Force Mono
  putShadow();                            // Write to registers
}	
//-----------------------------------------------------------------------------------------------------------------------------------
// Get Mono Status
//-----------------------------------------------------------------------------------------------------------------------------------
bool	Si4703::getMono(void)
{
  getShadow();                              // Read the current register set
  return (shadow.reg.POWERCFG.bits.MONO);   // return status
}	
//-----------------------------------------------------------------------------------------------------------------------------------
// Set Audio Mute
//-----------------------------------------------------------------------------------------------------------------------------------
void	Si4703::setMute(bool en)
{
  getShadow();                              // Read the current register set
  shadow.reg.POWERCFG.bits.DMUTE = en;      // 0= Mute disabled
  putShadow();                              // Write to registers
}	
//-----------------------------------------------------------------------------------------------------------------------------------
// get Audio Mute
//-----------------------------------------------------------------------------------------------------------------------------------
bool	Si4703::getMute(void)
{
  getShadow();                              // Read the current register set
  return (shadow.reg.POWERCFG.bits.DMUTE);  // return status
}	
//-----------------------------------------------------------------------------------------------------------------------------------
// Set Extended Volume Range
//-----------------------------------------------------------------------------------------------------------------------------------
void	Si4703::setVolExt(bool en)
{
  getShadow();                              // Read the current register set
  shadow.reg.SYSCONFIG3.bits.VOLEXT = en;   // 0=disabled (default)
  putShadow();                              // Write to registers
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Get Extended Volume Range
//-----------------------------------------------------------------------------------------------------------------------------------
bool	Si4703::getVolExt(void)
{
  getShadow();                               // Read the current register set
  return (shadow.reg.SYSCONFIG3.bits.VOLEXT);// return status
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Get Current Volume
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::getVolume(void)
{
  getShadow();                                // Read the current register set
  return(shadow.reg.SYSCONFIG2.bits.VOLUME);
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Set Volume
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::setVolume(int volume)
{
  getShadow();                                // Read the current register set
  if (volume < 0 ) volume = 0;                // Accepted Volume value 0-15
  if (volume > 15) volume = 15;               // Accepted Volume value 0-15
  shadow.reg.SYSCONFIG2.bits.VOLUME = volume; // Set volume to 0
  putShadow();                                // Write to registers
  return(getVolume());
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Increment Volume
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::incVolume(void)
{
  return(setVolume(getVolume()+1));
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Decrement Volume
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::decVolume(void)
{
  return(setVolume(getVolume()-1));
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Reads the current channel from READCHAN
// Returns a number like 974 for 97.4MHz
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::getChannel()
{
  getShadow();                                // Read the current register set
  
  // Freq = Spacing * Channel + Bottom of Band.
  return (_bandSpacing * shadow.reg.READCHAN.bits.READCHAN + _bandStart);  
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Sets Channel frequency
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::setChannel(int freq)
{
  if (freq > _bandEnd)    freq = _bandEnd;    // check upper limit
  if (freq < _bandStart)  freq = _bandStart;  // check lower limit

  // Freq     = Spacing * Channel + bandStart.
  // Channel  = (Freq - bandStart) / Spacing
  getShadow();                              // Read the current register set
  shadow.reg.CHANNEL.bits.CHAN  = (freq - _bandStart) / _bandSpacing;
  shadow.reg.CHANNEL.bits.TUNE  = 1;        // Set the TUNE bit to start
  putShadow();                              // Write to registers

  if (shadow.reg.SYSCONFIG1.bits.STCIEN == 0) // Select method Interrupt or STC
    while(!getSTC());                         // Wait for the si4703 to set the STC
  else
    {
      // Wait for interrupt indicating STC (Seek/Tune Complete)
      // TODO:
    }

  getShadow();                              // Read the current register set
  shadow.reg.CHANNEL.bits.TUNE  =0;         // Clear Tune bit
  putShadow();                              // Write to registers
  while(getSTC());                          // Wait for the si4703 to clear the STC

  return getChannel();
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Increment frequency one band step
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::incChannel(void)
{
  return setChannel(getChannel() + _bandSpacing); // Increment frequency one band step
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Decrement frequency one band step
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::decChannel(void)
{
  return setChannel(getChannel() - _bandSpacing); // Decrement frequency one band step
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Get STC status
//-----------------------------------------------------------------------------------------------------------------------------------
bool Si4703::getSTC(void)
{
  getShadow();                                  // Read the current register set
  return(shadow.reg.STATUSRSSI.bits.STC);
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Seeks the next available station
// Returns freq if seek succeeded
// Returns zero if seek failed
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::seek(byte seekDirection){

  getShadow();                                      // Read the current register set
  shadow.reg.POWERCFG.bits.SEEKUP = seekDirection;  // Seek direction = UP/Down
  shadow.reg.POWERCFG.bits.SEEK   = 1;              // Start seek
  putShadow();                                      // Write to registers to start seeking

  if (shadow.reg.SYSCONFIG1.bits.STCIEN == 0)       // Select method Interrupt or STC
    {
      while(!getSTC())                              // Wait for the si4703 to set the STC
      {
        delay(40);
        // you can show READCHAN value as a seek progress here
        // TODO:
      }
    }
  else
    {
      // Wait for interrupt indicating STC (Seek/Tune Complete)
      // TODO:
    }
  
  getShadow();                                      // Read the current register set
  bool sfbl = shadow.reg.STATUSRSSI.bits.SFBL;      // Save SFBL status
  shadow.reg.POWERCFG.bits.SEEK   = 0;              // Stop seek
  putShadow();                                      // Write to registers
  while(getSTC());                                  // Wait for the si4703 to clear the STC

  if(sfbl)  return(0);   // Failure: SFBL is indicating we hit a band limit or failed to find a station
  return getChannel();                              // Success: return new frequency
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
// Get Sterio current value
//-----------------------------------------------------------------------------------------------------------------------------------
bool Si4703::getST(void)
{
  getShadow();                              // Read the current register set
  return(shadow.reg.STATUSRSSI.bits.ST);    // Return ST value
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Read RDS
//-----------------------------------------------------------------------------------------------------------------------------------
void Si4703::readRDS(void)
{ 
  // TODO:
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Writes GPIO1-GPIO3
//-----------------------------------------------------------------------------------------------------------------------------------
	void	Si4703::writeGPIO(int GPIO, int val)
{
  getShadow();    // Read the current register set

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
  
  putShadow();  // Write to registers
}

//-----------------------------------------------------------------------------------------------------------------------------------
// Get DeviceID:Part Number
//-----------------------------------------------------------------------------------------------------------------------------------
int	Si4703::getPN()
{
  getShadow();    // Read the current register set
  return(shadow.reg.DEVICEID.bits.PN);
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Get DeviceID:Manufacturer ID
//-----------------------------------------------------------------------------------------------------------------------------------
int	Si4703::getMFGID()
{
  getShadow();    // Read the current register set
  return(shadow.reg.DEVICEID.bits.MFGID);
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Get ChipID:Chip Version
//-----------------------------------------------------------------------------------------------------------------------------------
int	Si4703::getREV()
{
  getShadow();    // Read the current register set
  return(shadow.reg.CHIPID.bits.REV);
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Get ChipID:Device
//-----------------------------------------------------------------------------------------------------------------------------------
int	Si4703::getDEV()
{
  getShadow();    // Read the current register set
  return(shadow.reg.CHIPID.bits.DEV);
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Get ChipID:Firmware Version
//-----------------------------------------------------------------------------------------------------------------------------------
int	Si4703::getFIRMWARE()
{
  getShadow();    // Read the current register set
  return(shadow.reg.CHIPID.bits.FIRMWARE);
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Get Band Start Frequency
//-----------------------------------------------------------------------------------------------------------------------------------
int	Si4703::getBandStart()
{
  return(_bandStart);
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Get Band End Frequency
//-----------------------------------------------------------------------------------------------------------------------------------
int	Si4703::getBandEnd()
{
  return(_bandEnd);
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Get Band Step
//-----------------------------------------------------------------------------------------------------------------------------------
int	Si4703::getBandSpace()
{
  return(_bandSpacing);
}
//-----------------------------------------------------------------------------------------------------------------------------------
// Get RSSI current value
//-----------------------------------------------------------------------------------------------------------------------------------
int Si4703::getRSSI(void)
{
  getShadow();                              // Read the current register set
  return(shadow.reg.STATUSRSSI.bits.RSSI);  // Return RSSI value
}