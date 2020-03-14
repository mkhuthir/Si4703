/* 

Library Modified by Muthanna Alwahash 2020

This library based on the code produced by Nathan Seidle and Simon Monk.

Remarks:

	* The Si4703 ACKs the first byte, and NACKs the 2nd byte of a read.
	* Don't updateRegisters without first readRegisters.

*/

#ifndef Si4703_h
#define Si4703_h

#include "Arduino.h"

//------------------------------------------------------------------------------------------------------------

// Band Select
static const uint8_t  	BAND_US_EU		= 0x00;	// 87.5–108 MHz (US / Europe, Default)
static const uint8_t  	BAND_JPW		= 0x01;	// 76–108 MHz (Japan wide band)
static const uint8_t  	BAND_JP			= 0x10;	// 76–90 MHz (Japan)

// De-emphasis
static const uint8_t	DE_75us			= 0x0;	// De-emphasis 75 μs. Used in USA (default)
static const uint8_t	DE_50us			= 0x1;	// De-emphasis 50 μs. Used in Europe, Australia, Japan.

// Channel Spacing
static const uint8_t  	SPACE_200KHz	= 0x00;	// 200 kHz (US / Australia, Default)
static const uint8_t  	SPACE_100KHz 	= 0x01;	// 100 kHz (Europe / Japan)
static const uint8_t  	SPACE_50KHz  	= 0x10;	//  50 kHz (Other)

// GPIO1-3 Pins
static const uint8_t  	GPIO1			= 1;	// GPIO1
static const uint8_t  	GPIO2			= 2;	// GPIO2
static const uint8_t  	GPIO3			= 3;	// GPIO3

// GPIO1-3 Possible Values
static const uint8_t 	GPIO_Z			= 0x00;	// High impedance (default)
static const uint8_t 	GPIO_I			= 0x01;	// GPIO1-Reserved, GPIO2-STC/RDS int, or GPIO3-Mono/Sterio Indicator
static const uint8_t 	GPIO_Low		= 0x10;	// Low output (GND level)
static const uint8_t 	GPIO_High		= 0x11;	// High output (VIO level)

// Seek FM Impulse Detection Threshold
static const uint8_t 	SKCNT_DIS		= 0x0000; // disabled (default)
static const uint8_t 	SKCNT_MAX		= 0x0001; // max (most stops)
static const uint8_t 	SKCNT_MIN		= 0x1111; // min (fewest stops)

// Seek SNR Threshold
static const uint8_t 	SKSNR_DIS		= 0x0000; // disabled (default)
static const uint8_t 	SKSNR_MIN		= 0x0001; // min (most stops)
static const uint8_t 	SKSNR_MAX		= 0x1111; // max (fewest stops)

// Softmute Attenuation
static const uint8_t 	SMA_16dB		=0x00;	// Softmute Attenuation 16dB (default)
static const uint8_t 	SMA_14dB		=0x01;	// Softmute Attenuation 14dB
static const uint8_t 	SMA_12dB		=0x10;	// Softmute Attenuation 12dB
static const uint8_t 	SMA_10dB		=0x11;	// Softmute Attenuation 10dB

// Softmute Attack/Recover Rate
static const uint8_t 	SMRR_Fastest	=0x00;	// Softmute Attack/Recover Rate = Fastest
static const uint8_t 	SMRR_Fast		=0x01;	// Softmute Attack/Recover Rate = Fast
static const uint8_t 	SMRR_Slow		=0x10;	// Softmute Attack/Recover Rate = Slow
static const uint8_t 	SMRR_Slowest	=0x11;	// Softmute Attack/Recover Rate = Slowest

//------------------------------------------------------------------------------------------------------------

class Si4703
{
//------------------------------------------------------------------------------------------------------------
  public:
    Si4703(	int resetPin, 				// Reset pin
			int sdioPin,				// I2C Data IO Pin
			int sclkPin,				// I2C Clock Pin
			int stcIntPin);				// Seek/Tune Complete Pin

	int		getDeviceID();				// 
	int		getChipID();				//
	
    void	powerUp();					// Power Up Device
	void	powerDown();				// Power Down Device to save power

	int 	getChannel();				// Get 3 digit channel number
	int		setChannel(int freq);  		// Set 3 digit channel number
	int		incChannel(void);			// Increment Channel Frequency one band step
	int		decChannel(void);			// Decrement Channel Frequency one band step
	

	void 	setSeekMode();				// Set Seek Mode
	int 	seekUp(); 					// Seeks up and returns the tuned channel or 0
	int 	seekDown(); 				// Seeks down and returns the tuned channel or 0

	void	setVolume(int volume); 		// Sets volume value 0 to 15

	void	readRDS(char* message,		// Reads RDS, message should be at least 9 chars, result will be null terminated.
					long timeout);		// timeout in milliseconds

	void	writeGPIO(int GPIO, 		// Write to GPIO1,GPIO2, and GPIO3
					  int val); 		// values can be GPIO_Z, GPIO_I, GPIO_Low, and GPIO_High

//------------------------------------------------------------------------------------------------------------
  private:
    int  	_rstPin;					// Reset pin
	int  	_sdioPin;					// I2C Data IO Pin
	int  	_sclkPin;					// I2C Clock Pin
	int  	_stcIntPin;					// Seek/Tune Complete Pin

	// Freq (MHz) = Spacing (kHz) * Channel + Bottom of Band (MHz).

	int		bandStart 	= 875;			// Bottom of Band (MHz)
	int		bandEnd		= 1080;			// Top of Band (MHz)
	int		bandSpacing	= 1;			// Band Spacing (MHz)

	void 	si4703_init();				// init class

	void	getShadow();				// Read registers to shadow 
	byte 	putShadow();				// Write shadow to registers

	int 	seek(byte seekDirection);

	// I2C interface
	static const int  		I2C_ADDR		= 0x10; // I2C address of Si4703 - note that the Wire function assumes non-left-shifted I2C address, not 0b.0010.000W
	static const uint16_t  	I2C_FAIL_MAX 	= 10; 	// This is the number of attempts we will try to contact the device before erroring out

	static const uint16_t  	SEEK_DOWN 		= 0; 	// Direction used for seeking. Default is down
	static const uint16_t  	SEEK_UP 		= 1;

	// Registers shadow
	//------------------------------------------------------------------------------------------------------------
	union DEVICEID_t	// Register 0x00
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	MFGID	:12;	// Part Number
			uint8_t 	PN		:4;		// Manufacturer ID
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	union CHIPID_t		// Register 0x01
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	FIRMWARE:6;		// Firmware Version
			uint16_t 	DEV		:4;		// Device
			uint16_t 	REV		:6;		// Chip Version
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	union POWERCFG_t	// Register 0x02
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	ENABLE	:1;		// Powerup Enable
			uint16_t			:5;		// Reserved
			uint16_t	DISABLE :1;		// Powerup Disable
			uint16_t			:1;		// Reserved
			uint16_t	SEEK	:1;		// Seek Disable/Enable
			uint16_t	SEEKUP	:1;		// Seek Direction Down/Up
			uint16_t	SKMODE	:1;		// Seek Mode Wrap/Stop
			uint16_t	RDSM	:1;		// RDS Mode Standard/Verbose
			uint16_t			:1;		// Reserved
			uint16_t	MONO	:1;		// Mono Select Stereo/Mono
			uint16_t	DMUTE	:1;		// Mute Disable Enable/Disable
			uint16_t	DSMUTE	:1;		// Softmute Disable Enable/Disable

		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	union CHANNEL_t		// Register 0x03 
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	CHAN	:10;	// Channel Select (FreqMHz = SpacingMHZ x Channel + (87.5MHZ or 76MHz))
			uint16_t			:5;		// Reserved
			uint16_t	TUNE	:1;		// Tune Disable/Enable
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	union SYSCONFIG1_t	// Register 0x04
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	GPIO1	:2;		// General Purpose I/O 1
			uint16_t	GPIO2	:2;		// General Purpose I/O 2
			uint16_t	GPIO3	:2;		// General Purpose I/O 3
			uint16_t	BLNDADJ	:2;		// Stereo/Mono Blend Level Adjustment
			uint16_t			:2;		// Reserved
			uint16_t	AGCD	:1;		// AGC Disable Enable/Disable
			uint16_t	DE		:1;		// De-emphasis	75us/50us
			uint16_t	RDS		:1;		// RDS Enable Disable/Enable
			uint16_t			:1;		// Reserved
			uint16_t	STCIEN 	:1;		// Seek/Tune Complete Interrupt Enable Disable/Enable
			uint16_t	RDSIEN	:1;		// RDS Interrupt Enable Disable/Enable
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	union SYSCONFIG2_t	// Register 0x05
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	VOLUME	:4;		// Volume 0-15 (-30dBFS with VOLEXT)
			uint16_t	SPACE	:2;		// Channel Spacing 200/100/50 Khz
			uint16_t	BAND	:2;		// Band Select US/JPW/JP
			uint16_t	SEEKTH	:8;		// RSSI Seek Threshold 0x00-0x7F
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	union SYSCONFIG3_t	// Register 0x06
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	SKCNT	:4;		// Seek FM Impulse Detection Threshold
			uint16_t	SKSNR	:4;		// Seek SNR Threshold
			uint16_t	VOLEXT	:1;		// Extended Volume Range Disable/Enable
			uint16_t			:3;		// Reserved
			uint16_t	SMUTEA	:2;		// Softmute Attenuation
			uint16_t	SMUTER	:2;		// Softmute Attack/Recover Rate
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	union TEST1_t		// Register 0x07
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t			:14;	// Reserved
			uint16_t	AHIZEN	:1;		// Audio High-Z Enable Disable/Enable
			uint16_t	XOSCEN	:1;		// Crystal Oscillator Enable Disable/Enable
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	union TEST2_t		// Register 0x08
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t			:16;	// Reserved
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	union BOOTCONFIG_t	// Register 0x09
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t			:16;	// Reserved
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	union STATUSRSSI_t	// Register 0x0A
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	RSSI 	:8;		// RSSI (Received Signal Strength Indicator)
			uint16_t	ST		:1;		// Stereo Indicator. Mono/Stereo
			uint16_t	BLERA 	:2;		// RDS Block A Errors.
			uint16_t	RDSS 	:1;		// RDS Synchronized
			uint16_t	AFCRL	:1;		// AFC Rail.
			uint16_t	SFBL 	:1;		// Seek Fail/Band Limit.
			uint16_t	STC 	:1;		// Seek/Tune Complete.
			uint16_t	RDSR 	:1;		// RDS Ready.
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	union READCHAN_t	// Register 0x0B
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	READCHAN :10;	// Read Channel
			uint16_t	BLERD 	:2;		// RDS Block D Errors.
			uint16_t	BLERC 	:2;		// RDS Block C Errors.
			uint16_t	BLERB	:2;		// RDS Block B Errors.
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	union RDSA_t		// Register 0x0C
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	RDSA 	:16;	// RDSA
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	union RDSB_t		// Register 0x0D
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	RDSB 	:16;	// RDSB
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	union RDSC_t		// Register 0x0E
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	RDSC 	:16;	// RDSC
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	union RDSD_t		// Register 0x0F
	{
		uint16_t 	word;

		struct bits
		{
			uint16_t	RDSD 	:16;	// RDSD
		} 			bits;
	};
	//------------------------------------------------------------------------------------------------------------
	union shadow_t
	{
		uint16_t	word[16];	// 32 bytes = 16 x 16 bits Registers

		struct reg
		{

			STATUSRSSI_t	STATUSRSSI;	// Register 0x0A - 00
			READCHAN_t		READCHAN;	// Register 0x0B - 01
			RDSA_t			RDSA;		// Register 0x0C - 02
			RDSB_t			RDSB;		// Register 0x0D - 03
			RDSC_t			RDSC;		// Register 0x0E - 04
			RDSD_t			RDSD;		// Register 0x0F - 05

			DEVICEID_t 		DEVICEID;	// Register 0x00 - 06
			CHIPID_t		CHIPID;		// Register 0x01 - 07
			POWERCFG_t 		POWERCFG;	// Register 0x02 - 08
			CHANNEL_t		CHANNEL;	// Register 0x03 - 09
			SYSCONFIG1_t	SYSCONFIG1;	// Register 0x04 - 10
			SYSCONFIG2_t	SYSCONFIG2;	// Register 0x05 - 11
			SYSCONFIG3_t	SYSCONFIG3;	// Register 0x06 - 12
			TEST1_t			TEST1;		// Register 0x07 - 13
			TEST2_t			TEST2;		// Register 0x08 - 14
			BOOTCONFIG_t	BOOTCONFIG;	// Register 0x09 - 15


		} 			reg;

	} shadow;							// There are 16 registers, each 16 bits large;
};
#endif