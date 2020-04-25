// AppTmp.cpp
//
//*********************************************************************************************************
//*	Edit History, started June, 2011
//*	please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//*	Sep  15,	2013	(fc) created;
//*
//*********************************************************************************************************

#include "HT1632.h"
#include "AppTmp.h"
#include "WiseClock.h"
#include "DS3231.h"		// get temp directly;


//*********************************************************************************************************
void CAppTmp::init(byte mode/*=0*/)
{
	useCelsius = mode;

	saveUseCelsius();

	wiseClock.buttonsInUse  = SET_BUTTON + PLUS_BUTTON;
	wiseClock.plusButtonCnt = getTempOffset();
	wiseClock.setButtonCnt  = 0;

	clearDisplay();
}


//*********************************************************************************************************
int16_t CAppTmp::run()
{
	if (wiseClock.plusButtonCnt > 9)						// Temperature correction +9 -> -9;
		wiseClock.plusButtonCnt = -9;

	displayStaticLine_P(PSTR("Temp"), 0, GREEN);
	ht1632_putchar( 7, 8, (wiseClock.plusButtonCnt < 0) ? '-' : '+', ORANGE);
	ht1632_putchar(13, 8, '0' + abs(wiseClock.plusButtonCnt), ORANGE);
	
	if (wiseClock.setButtonCnt == 0)						// wait till Set button pressed;
		return(100);
		
	wiseClock.buttonsInUse	= 	0;

	saveTempOffset(wiseClock.plusButtonCnt);

	wiseClock.plusButtonCnt = 0;
	
	wiseClock.restoreCrtApp();
	wiseClock.checkCrtApp();

	return(100);
}


//*********************************************************************************************************
void CAppTmp::saveUseCelsius()
{
	wiseClock.saveUserSetting(useCelsiusLoc, useCelsius); 
}


//*********************************************************************************************************
boolean CAppTmp::getUseCelsius()
{
	return boolean (wiseClock.readUserSetting(useCelsiusLoc));
}


//*********************************************************************************************************
//
//  return temperature scaled by 4 -- after warming up the temp is about 3 degrees too high!;
//
//
int CAppTmp::getTemperature()
{
        return RTC.getTemperature() + (getTempOffset() * 4);
}      


//*********************************************************************************************************
int CAppTmp::getTempOffset()
{
	int i = wiseClock.readUserSetting(tempOffsetLoc);
	if (i & 0x80)				// unsigned byte to signed 2 byte int;
		i = i | 0xff00;
	return i;
}


//*********************************************************************************************************
void CAppTmp::saveTempOffset(byte tempCorrection)
{
	wiseClock.saveUserSetting(tempOffsetLoc, tempCorrection);
}



CAppTmp appTemperature;


