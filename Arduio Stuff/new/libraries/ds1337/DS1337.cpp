//*************************************************************************
// from http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1191209057
//*************************************************************************
//

#include <WProgram.h>

extern "C" {
	#include <../Wire/Wire.h>
}

#include "DS1337.h"


/*
// Internal macro for storing month days!
#ifndef GETMONTHDAYS
uint16_t PROGMEM monthcount[] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
#define	GETMONTHDAYS(index) (pgm_read_word_near(monthcount + index))
#endif
*/

int monthcount[] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
#define	GETMONTHDAYS(index) (monthcount[index])


#ifndef CI
	#define CI(reg,bit)		(reg & ~(bit))
#endif

#ifndef SI
	#define SI(reg,bit)		(reg | bit)
#endif


DS1337::DS1337()
{		
	clockExists = false;
	alarmId		= false;
	
#ifdef WIRE_LIB_SCAN_MOD
	/*if (!Wire.isStarted()) */Wire.begin();
#else
	Wire.begin();
#endif
}

DS1337 RTC = DS1337();

#ifdef WIRE_LIB_SCAN_MOD
int8_t DS1337::Init(void)
{
	delay(500); //Account for the crystal startup time
	
	// Check address and returns false is there is an error
	if (Wire.checkAddress(DS1337_ADDR)) {
		// Possibly set the default registers here
		
		clockExists	= true;
		
		// Start the oscillator if need
		if (getRegisterBit(DS1337_SP, DS1337_SP_EOSC) || getRegisterBit(DS1337_STATUS, DS1337_STATUS_OSF))
		{
			clockStart();
			
		#if defined(DS1337_USE_ALARM_INTERRUPTS) && !defined(DS1337_USE_SQW_OUTPUT)
			setRegister(DS1337_SP, DS1337_SQW_INTCN);
		#endif
		}
		
		return DS1337_ADDR;
	} else clockExists	= false;
	
	return -1;	
}
#else
int8_t DS1337::Init(void)
{
	delay(500); //Account for the crystal startup time
	
	clockExists	= true;
	
	// Start the oscillator if need
	if (getRegisterBit(DS1337_SP, DS1337_SP_EOSC) || getRegisterBit(DS1337_STATUS, DS1337_STATUS_OSF))
	{	
		clockStart();
	}
	
	return DS1337_ADDR;
}
#endif

void DS1337::setRegister(uint8_t registerNumber, uint8_t registerMask)
{
	writeRegister(registerNumber, SI(getRegister(registerNumber), registerMask));
}

void DS1337::unsetRegister(uint8_t registerNumber, uint8_t registerMask)
{
	writeRegister(registerNumber, CI(getRegister(registerNumber), registerMask));
}

void DS1337::writeRegister(uint8_t registerNumber, uint8_t registerValue)
{
	if (!clockExists) return;
	
	Wire.beginTransmission(DS1337_ADDR);
	Wire.send(registerNumber);
	
	Wire.send(registerValue);
	
	Wire.endTransmission();
}

uint8_t DS1337::getRegister(uint8_t registerNumber)
{
	if (!clockExists) return 0;
	
	Wire.beginTransmission(DS1337_ADDR);
	Wire.send(registerNumber);
	Wire.endTransmission();
	
	Wire.requestFrom(DS1337_ADDR, 1);
	
	while (!Wire.available());
	
	return Wire.receive();
}

void DS1337::clockStart(void)
{
	// Start the oscillator
	unsetRegister(DS1337_SP, DS1337_SP_EOSC);
	
	// Reset the status register
	writeRegister(DS1337_STATUS, B00000000);
}

void DS1337::clockRead(void)
{
	if (!clockExists) return;
	
	Wire.beginTransmission(DS1337_ADDR);
	Wire.send(0x00);
	Wire.endTransmission();

	Wire.requestFrom(DS1337_ADDR, 7);
	while (!Wire.available());
	
	for(int i=0; i<7; i++)
	{
		if (Wire.available())
			rtc_bcd[i]	= Wire.receive();
	}
}

void DS1337::clockSave(void)
{
	if (!clockExists) return;
	
	// Stop the clock
	clockStop();
	
	Wire.beginTransmission(DS1337_ADDR);
	Wire.send(0x00);
	
	for(int i=0; i<7; i++)
	{
		Wire.send(rtc_bcd[i]);
	}
	
	Wire.endTransmission();
	
	// Restart the oscillator
	clockStart();
}

void DS1337::clockGet(uint16_t *rtc)
{
	clockRead();
	
	for(int i=0;i<8;i++)  // cycle through each component, create array of data
	{
		rtc[i]=clockGet(i, 0);
	}
}

uint16_t DS1337::clockGet(uint8_t c, boolean refresh)
{
	if(refresh) clockRead();
	
	int timeValue=-1;
	switch(c)
	{
		case DS1337_SEC:
			timeValue = bcdToBin(DS1337_SEC, DS1337_HI_SEC);
		break;
		case DS1337_MIN:
			timeValue = bcdToBin(DS1337_MIN, DS1337_HI_MIN);
		break;
		case DS1337_HR:
			timeValue = bcdToBin(DS1337_HR, DS1337_HI_HR);
		break;
		case DS1337_DOW:
			timeValue = rtc_bcd[DS1337_DOW] & DS1337_LO_DOW;
		break;
		case DS1337_DATE:
			timeValue = bcdToBin(DS1337_DATE, DS1337_HI_DATE);
		break;
		case DS1337_MTH:
			timeValue = CI(bcdToBin(DS1337_MTH, DS1337_HI_MTH), DS1337_LO_CNTY);
		break;
		case DS1337_YR:
			timeValue = bcdToBin(DS1337_YR, DS1337_HI_YR)+(1900 + (rtc_bcd[DS1337_MTH] & DS1337_LO_CNTY ? 100 : 0));
		break;
		case DS1337_CNTY:
			timeValue = rtc_bcd[DS1337_MTH] & DS1337_LO_CNTY>>7;
		break;
	} // end switch
	
	return timeValue;
}

#ifdef DS1337_USE_BCD_CLOCKSET
void DS1337::clockSet(uint8_t timeSection, uint16_t timeValue)
{
	switch(timeSection)
	{
		case DS1337_SEC:
		if(timeValue<60)
		{
			rtc_bcd[DS1337_SEC]		= binToBcd(timeValue);
		}
		break;
		
		case DS1337_MIN:
		if(timeValue<60)
		{
			rtc_bcd[DS1337_MIN]		= binToBcd(timeValue);
		}
		break;
		
		case DS1337_HR:
		// TODO : AM/PM  12HR/24HR
		if(timeValue<24)
		{
			rtc_bcd[DS1337_HR]		= binToBcd(timeValue);
		}
		break;
		
		case DS1337_DOW: 
		if(timeValue<8)
		{
			rtc_bcd[DS1337_DOW]		= timeValue;
		}
		break;
		
		case DS1337_DATE:
		if(timeValue<31)
		{
			rtc_bcd[DS1337_DATE]	= binToBcd(timeValue);
		}
		break;
		
		case DS1337_MTH:
		if(timeValue<13)
		{
			rtc_bcd[DS1337_MTH]		= SI(CI(binToBcd(timeValue),DS1337_LO_CNTY), (rtc_bcd[DS1337_MTH] & DS1337_LO_CNTY));
		}
		break;
		
		case DS1337_YR:
		if(timeValue<1000)
		{
			rtc_bcd[DS1337_YR]		= binToBcd(timeValue);
		}
		break;
		
		case DS1337_CNTY:
		if (timeValue > 0)
		{
			rtc_bcd[DS1337_MTH]		= SI(rtc_bcd[DS1337_MTH], DS1337_LO_CNTY);
		} else {
			rtc_bcd[DS1337_MTH]		= CI(rtc_bcd[DS1337_MTH], DS1337_LO_CNTY);
		}
		break;
	} // end switch
		
	clockSave();
}
#endif

#ifdef DS1337_USE_GET_UTS
uint32_t DS1337::calculateUTS(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec)
{
	// Number of days per month
	// Compute days
	tt = (year - 1970) * 365 + GETMONTHDAYS(month) + day - 1;
	
	// Compute for leap year
	for (month <= 2 ? year-- : 0; year >= 1970; year--)
		if (ISLEAP(year))
			tt++;
	
	// Plus the time
	tt = sec + 60 * (min + 60 * (tt * 24 + hour - RTC_GMT_OFFSET));
	
	return tt;
}
#endif

#ifdef DS1337_USE_SET_UTS
void DS1337::clockSetWithUTS(uint32_t unixTimeStamp, boolean correctedTime)
{
	int8_t		ii;
	uint16_t	year;
	uint16_t	*yrPtr	= &year;
	uint16_t	thisYear;
	uint16_t	*tyPtr	= &thisYear;
#if defined(RTC_DST_TYPE)
	uint8_t		thisDate;
	uint8_t		*tdPtr	= &thisDate;
#endif
	
	// Serial.print((uint32_t)unixTimeStamp, DEC); delay(5); SPrint(" ");
	
	/**
	 * Calculate GMT and DST
	**/
#if defined(RTC_GMT_OFFSET)
	if (!correctedTime) unixTimeStamp = (uint32_t)unixTimeStamp + (RTC_GMT_OFFSET * 3600);
#endif
	
	// Years
	tt			= (uint32_t)unixTimeStamp / 3600 / 24 / 365;
	year		= tt + 1970;
	
	thisYear	= year;
	
	// Set the century bit
	if (tt > 30) {
		rtc_bcd[DS1337_MTH]	= SI(rtc_bcd[DS1337_MTH], DS1337_LO_CNTY);
		tt-= 30;
	} else {
		rtc_bcd[DS1337_MTH]	= CI(rtc_bcd[DS1337_MTH], DS1337_LO_CNTY);
	}
	
	// Set the year
	rtc_bcd[DS1337_YR]		= binToBcd(tt);
	
	// Number of days left in the year
	// Serial.print((uint32_t)unixTimeStamp, DEC); delay(5); SPrint(" "); delay(5);
	// Serial.print((uint32_t)unixTimeStamp%31536000, DEC); delay(5); SPrint(" "); delay(5);
	tt = ((uint32_t)(uint32_t)unixTimeStamp%31536000 / 3600 / 24) + 1;
	
	// Serial.print(tt, DEC); delay(5); SPrint(" "); delay(5);
	
	// leap year correction
	for (year--; year > 1970; year--) {
		if (ISLEAP(year))
		{
			tt--;
		}
	}
	
	free(yrPtr);
	
	// Serial.print(tt, DEC); SPrint(" ");
	
	// Grab the number of previous days, account for leap years, and save the month
	for (ii = 1; ii < 12; ii++)
	{
		if (( GETMONTHDAYS(ii+1) + ((ISLEAP(thisYear) && ii >= 2) ? 1 : 0) ) > tt)
		{
			rtc_bcd[DS1337_MTH]		= SI(CI(binToBcd(ii), DS1337_LO_CNTY), (rtc_bcd[DS1337_MTH] & DS1337_LO_CNTY));
			break;
		}
	}
	// Serial.print(ii, DEC);
	// SPrint(" ");
	
	// Date
#if defined(RTC_DST_TYPE)
	if (!correctedTime) {
		thisDate = tt - (GETMONTHDAYS(ii) + ((ISLEAP(thisYear) && ii >= 2) ? 1 : 0));
	}
#endif
	
	// Date
	rtc_bcd[DS1337_DATE]	= binToBcd( tt - (GETMONTHDAYS(ii) + ((ISLEAP(thisYear) && ii >= 2) * 1)) );
	// Serial.print( tt - (GETMONTHDAYS(ii) + ((ISLEAP(thisYear) && ii >= 2) * 1)) , DEC);
	// SPrint(" ");
	
	// Day of the week
	rtc_bcd[DS1337_DOW]		= ((tt)%7 + 1) & DS1337_LO_DOW;
	// Serial.print(((tt)%7 + 1) & DS1337_LO_DOW, DEC);
	// SPrint(" ");
	
	// Hour
	tt = (uint32_t)unixTimeStamp%86400 / 3600;
	rtc_bcd[DS1337_HR]		= binToBcd(tt);
	// Serial.print(tt, DEC);
	// SPrint(" ");
	
#if defined(RTC_DST_TYPE)
	if (!correctedTime) {
		uint8_t dstStartMo, dstStopMo, dstStart, dstStop;
		uint8_t	*dstStartMoPtr	= &dstStartMo;
		uint8_t	*dstStopMoPtr	= &dstStopMo;
		uint8_t	*dstStartPtr	= &dstStart;
		uint8_t	*dstStopPtr		= &dstStop;
		
	#ifndef RTC_CHECK_OLD_DST
		dstStart	= DSTCALCULATION1(thisYear);
	#if RTC_DST_TYPE == 1
		dstStop		= DSTCALCULATION1(thisYear);	// EU DST
	#else
		dstStop		= DSTCALCULATION2(thisYear);	// US DST
	#endif
		dstStartMo	= 3;
		dstStopMo	= 11;
	#else
		if (thisYear < RTC_DST_OLD_YEAR) {
			dstStart	= ((2+6 * thisYear - (thisYear / 4) ) % 7 + 1);
			dstStop		= (14 - ((1 + thisYear * 5 / 4) % 7));
			dstStartMo	= 4;
			dstStopMo	= 10;
		} else {
			dstStart	= DSTCALCULATION1(thisYear)
		#if RTC_DST_TYPE == 1
		 	dstStop		= DSTCALCULATION1(thisYear)	// EU DST
		#else
			dstStop		= DSTCALCULATION2(thisYear);	// US DST
		#endif
			dstStartMo	= 3;
			dstStopMo	= 11;
		}
	#endif
		if (ii >= dstStartMo && ii <= dstStopMo)
		{
			if ( (ii < dstStopMo && (ii > dstStartMo || thisDate > dstStart || thisDate == dstStart && tt >= 2)) ||
				 (thisDate < dstStop || thisDate == dstStop && tt < 2) )
			{
				/**
				 * Free as much memory as possible before entering recursion
				**/
				free(tyPtr);
				free(tdPtr);
				free(dstStopPtr);
				free(dstStartPtr);
				free(dstStopMoPtr);
				free(dstStartMoPtr);
				
				clockSetWithUTS((uint32_t)unixTimeStamp + 3600, true);
				return;
			}
		}
	}
#endif
	
	// Minutes
	tt = (uint32_t)unixTimeStamp%3600 / 60;
	rtc_bcd[DS1337_MIN]		= binToBcd(tt);
	// Serial.print(tt, DEC);
	// SPrint(" ");
	
	// Seconds
	tt = ((uint32_t)unixTimeStamp%3600)%60;
	rtc_bcd[DS1337_SEC]		= binToBcd(tt);
	// Serial.print(tt, DEC);
	// SPrint(" ");
	
	// Save buffer to the RTC
	clockSave();
}
#endif

uint8_t	DS1337::bcdToBin(uint8_t index, uint8_t hi)
{
	return (10*((rtc_bcd[index] & hi)>>4))+(rtc_bcd[index] & DS1337_LO_BCD);
}
uint8_t	DS1337::binToBcd(uint8_t val)
{
	return ((((val)/10)<<4) + (val)%10);
}

/**
 * Alarm support functions
**/
#ifdef DS1337_USE_ALARMS
void DS1337::alarmSelect(boolean alarm)
{
	alarmId	= alarm;
	return;
}

#ifdef DS1337_USE_ALARMS_CALLBACK
void DS1337::alarmSetCallback(void (*userFunc)(void))
{
	DS1337callbackFunc[(alarmId ? 1 : 0)] = userFunc;
}

void DS1337::alarmUnsetCallback(void)
{
	DS1337callbackFunc[(alarmId ? 1 : 0)] = 0;
}

void DS1337::alarmChecks(void)
{
	if (getRegisterBit(DS1337_STATUS, DS1337_STATUS_A1F))
	{
		if (DS1337callbackFunc[0]) DS1337callbackFunc[0]();
		unsetRegister(DS1337_STATUS, DS1337_STATUS_A1F);
	}
	
	if (getRegisterBit(DS1337_STATUS, DS1337_STATUS_A2F))
	{
		if (DS1337callbackFunc[1]) DS1337callbackFunc[1]();
		unsetRegister(DS1337_STATUS, DS1337_STATUS_A2F);
	}
	
	return;
}
#endif

void DS1337::alarmSet(uint8_t timeSection, uint8_t timeValue)
{
	if (timeSection > DS1337_ALARM_DT)			return;
	
	if (alarmId == true) rtc_alarm[DS1337_SEC]	= 0;
	
	switch(timeSection)
	{
		case DS1337_SEC:
		if(alarmId == false && timeValue<60)
		{
			rtc_alarm[DS1337_SEC]			= (binToBcd(timeValue) & ~DS1337_ALARM_MASK) | (rtc_alarm[DS1337_SEC] & DS1337_ALARM_MASK);
		}
		break;
		
		case DS1337_MIN:
		if(timeValue < 60)
		{
			rtc_alarm[DS1337_MIN]			= (binToBcd(timeValue) & ~DS1337_ALARM_MASK) | (rtc_alarm[DS1337_MIN] & DS1337_ALARM_MASK);
		}
		break;
		
		case DS1337_HR:
		if(timeValue < 24)
		{
			rtc_alarm[DS1337_HR]			= (binToBcd(timeValue) & ~DS1337_ALARM_MASK) | (rtc_alarm[DS1337_HR] & DS1337_ALARM_MASK);
		}
		break;
		
		case DS1337_DOW: 
		if(timeValue < 31)
		{
			rtc_alarm[DS1337_DOW]			= (binToBcd(timeValue) & ~DS1337_ALARM_MASK & ~DS1337_ALARM_DT_MASK) | (rtc_alarm[DS1337_DOW] & DS1337_ALARM_MASK) | (rtc_alarm[DS1337_DOW] & DS1337_ALARM_DT_MASK);
		}
		break;
		
		case DS1337_ALARM_MODE:
		{
			if (alarmId) {
				timeValue					= (timeValue>>1)<<1;
			} else
				// A1M1
				rtc_alarm[DS1337_SEC]		= (rtc_alarm[DS1337_SEC] & ~DS1337_ALARM_MASK)	| DS1337_ALARM_MASK & timeValue<<7;
			
			// AM2
			rtc_alarm[DS1337_MIN]			= (rtc_alarm[DS1337_MIN] & ~DS1337_ALARM_MASK)	| DS1337_ALARM_MASK & timeValue<<6;
			// AM3
			rtc_alarm[DS1337_HR]			= (rtc_alarm[DS1337_HR] & ~DS1337_ALARM_MASK)	| DS1337_ALARM_MASK & timeValue<<5;
			// AM4
			rtc_alarm[DS1337_DOW]			= (rtc_alarm[DS1337_DOW] & ~DS1337_ALARM_MASK)	| DS1337_ALARM_MASK & timeValue<<4;
			
			if (timeValue == DS1337_ALARM_MCH_DOWHRMINSEC || timeValue == DS1337_ALARM_MCH_DOWHRMIN)
			{
				rtc_alarm[DS1337_DOW]		= (rtc_alarm[DS1337_DOW] & ~DS1337_ALARM_DT_MASK)	| DS1337_ALARM_DT_MASK;
			}
		}
		
		break;
		
		case DS1337_ALARM_DT:
		{
			rtc_alarm[DS1337_DOW]			= (rtc_alarm[DS1337_DOW] & ~DS1337_ALARM_DT_MASK) | (timeValue == 1 ? DS1337_ALARM_DT_MASK : 0);
		}
		break;
	} // end switch
	
	alarmSave();
}

boolean DS1337::alarmCheck(void)
{
	if (getRegisterBit(DS1337_STATUS, (alarmId == false ? DS1337_STATUS_A1F : DS1337_STATUS_A2F)))
	{
		unsetRegister(DS1337_STATUS, (alarmId == false ? DS1337_STATUS_A1F : DS1337_STATUS_A2F));
		
		return true;
	}
	
	return false;
}

void DS1337::alarmSave(void)
{
	Wire.beginTransmission(DS1337_ADDR);
	Wire.send((alarmId == false ? DS1337_ALARM1 : DS1337_ALARM2));
	
	for(uint8_t i=(alarmId == false ? 0 : 1); i<4; i++)
	{
		Wire.send(rtc_alarm[i]);
	}
	
	Wire.endTransmission();
	
	delay(5);
}

#ifdef DS1337_USE_ALARM_INTERRUPTS
void DS1337::alarmDisableInterrupts(void)
{
	unsetRegister(DS1337_SP, DS1337_ALARM_INT1);
	unsetRegister(DS1337_SP, DS1337_ALARM_INT2);
	
	return;
}

void DS1337::alarmSetInterrupt(void)
{
#ifndef DS1337_USE_SQW_OUTPUT
	if (alarmId) setRegister(DS1337_SP, DS1337_SQW_INTCN);
#endif
	
	setRegister(DS1337_SP, (alarmId == false ? DS1337_ALARM_INT1 : DS1337_ALARM_INT2));
	
	return;
}

void DS1337::alarmUnsetInterrupt(void)
{
	unsetRegister(DS1337_SP, (alarmId == false ? DS1337_ALARM_INT1 : DS1337_ALARM_INT2));
	
	return;
}
#endif
#endif

#ifdef DS1337_USE_SQW_OUTPUT
void DS1337::sqwEnable(void)
{
	unsetRegister(DS1337_SP, DS1337_SQW_INTCN);
	return;	
}

void DS1337::sqwDisable(void)
{
	setRegister(DS1337_SP, DS1337_SQW_INTCN);
	return;	
}

void DS1337::sqwSetRate(uint8_t sqwRate)
{
	writeRegister(DS1337_SP, (getRegisterSP() & ~DS1337_SQW_RS | sqwRate));
	
	return;
}
#endif

#ifdef DS1337_USE_OSC_INTEGRITY
void DS1337::clockIntegrityCallback(void (*userFunc)(void))
{
	DS1337callbackFunc[2]	= userFunc;
}
#endif

#if defined(DS1337_USE_ALARMS_CALLBACK) || defined(DS1337_USE_OSC_INTEGRITY)
void DS1337::clockChecks(void)
{
#ifdef DS1337_USE_OSC_INTEGRITY
	if (getRegisterBit(DS1337_STATUS, DS1337_STATUS_OSF) && !getRegisterBit(DS1337_SP, DS1337_SP_EOSC))
	{
		clockStop();
		
		if (DS1337callbackFunc[2]) DS1337callbackFunc[2]();
		
		return;
	}
#endif
#ifdef DS1337_USE_ALARMS_CALLBACK
	alarmChecks();
#endif
}
#endif

#ifdef DS1337_DEBUG
void DS1337::printRegisters(void)
{
	for(int ii=0;ii<0x10;ii++)
	{
		Serial.print("0x");
		Serial.print(ii, HEX);
		Serial.print(" ");
		Serial.println(getRegister(ii), BIN);
	}
	
	delay(200);
}
#endif
