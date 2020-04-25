// AppTimeClock.cpp

//*********************************************************************************************************
//*	Edit History, started Oct, 2011
//*	please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//*	Oct  15,	2011	(rp) first entry;
//*	May  02,	2012	(mcm) use #define for scroll prefix
//*	May  28,	2012	(mcm) restructure to eliminate delay() calls
//*
//*********************************************************************************************************

#include "UserConf.h"
#ifdef WANT_APP_TCLOK

#include "AppTimeClock.h"
#include "HT1632.h"
#include "WiseClock.h"
#include "AlarmClock.h"


void CAppTimeClock::init(byte mode /*=0*/)
{
	clearDisplay();
	wiseClock.plusButtonCnt = getCurrentTCProject();
	wiseClock.setButtonCnt = 0;
	tClockState = 100;
	needInit = false;
}


int16_t CAppTimeClock::run()
{
	unsigned int outTime;
	
	wiseClock.buttonsInUse = SET_BUTTON + PLUS_BUTTON;

	if (needInit)
	    init();

	if (tClockState == 100)
	{
		if (wiseClock.plusButtonCnt > 5)							
			wiseClock.plusButtonCnt = 1;

		displayStaticLine_P(PSTR("PROJ?"), 0, GREEN);
		ht1632_putchar(10, 8, 'P', ORANGE);
		ht1632_putchar(16, 8, '0' + wiseClock.plusButtonCnt, ORANGE);
		if (wiseClock.setButtonCnt == 0)						// wait till Set button pressed;
			return(100);

		wiseClock.setButtonCnt = 0;	
		tcProject = wiseClock.plusButtonCnt;
		saveCurrentTCProject();
		wiseClock.plusButtonCnt = 0;
		getTimeClockdetails(tcProject, &tClockState, &inTime, &totalTime);
		clearDisplay();
	}
	
	if (tClockState == 1)										// 1 = Project was started;
	{
		displayStaticLine_P(PSTR(" OUT "), 0, GREEN);
		outTime = alarmClock.hour * 60 + alarmClock.minute;
		if (outTime < inTime)
			outTime = outTime + (24 * 60);						// Project passing 00:00 hours at midnight;
		showSubTotal(totalTime + (outTime - inTime));
		if (wiseClock.setButtonCnt == 0)						// wait till Set button pressed;
			return(100);

		wiseClock.setButtonCnt = 0;	
		totalTime = totalTime + (outTime - inTime);
		saveTimeClockdetails(tcProject, 0, outTime, totalTime);
		displayStaticLine_P(PSTR("Total"), 0, GREEN);
#ifdef WANT_EVENT_LOG
		wiseClock.updateLogFile("PO", alarmClock.hour, alarmClock.minute, alarmClock.day, alarmClock.month, alarmClock.year - 2000, tcProject, totalTime);
#endif
		needInit = true;
		return(2000);
	}
			
	if (tClockState == 0)										// 0 = project was stopped;
	{
		if (wiseClock.plusButtonCnt > 1)							
			wiseClock.plusButtonCnt = 0;
		showSubTotal(totalTime);

		if (wiseClock.plusButtonCnt == 1)
			displayStaticLine_P(PSTR(" NEW "), 0, RED);
		else
			displayStaticLine_P(PSTR(" IN? "), 0, GREEN);
		if (wiseClock.setButtonCnt == 0)						// wait till Set button pressed;
			return(100);

		wiseClock.setButtonCnt = 0;	
		if (wiseClock.plusButtonCnt == 1)
			tClockState = 101;
	}		

	if (tClockState == 101)
	{
		if (wiseClock.plusButtonCnt > 1)							
			wiseClock.plusButtonCnt = 0;
		if (totalTime != 0)
		{		
			displayStaticLine_P(PSTR("Sure?"), 0, GREEN);
			if (wiseClock.plusButtonCnt == 1)				
				displayStaticLine_P(PSTR(" NO   "), 8, GREEN);
			else	
				displayStaticLine_P(PSTR(" YES  "), 8, RED);
			if (wiseClock.setButtonCnt == 0)					// wait till Set button pressed;
				return(100);

			wiseClock.setButtonCnt = 0;	
			if (wiseClock.plusButtonCnt == 1)
			{
				wiseClock.plusButtonCnt = 0;					// if not sure go back to previous state;
				tClockState = 0;
				return(100);
			}	
			memset(wiseClock.tempBuffer, ' ', BLANK_PREFIX);
			wiseClock.tempBuffer[BLANK_PREFIX] = 0;
			displayStaticLine(wiseClock.tempBuffer, 8, BLACK);
			totalTime = 0;
		}
	}		
		
	showSubTotal(totalTime);
	inTime = (alarmClock.hour * 60) + alarmClock.minute;
	saveTimeClockdetails(tcProject, 1, inTime, totalTime);
	displayStaticLine_P(PSTR("Start"), 0, ORANGE);
	needInit = true;
	return(2000);
}	
	
void CAppTimeClock::showSubTotal(unsigned int totalTime)
{
	int totalHours, totalMinutes;
	
	totalHours = totalTime / 60;
	totalMinutes = totalTime % 60;
	
	ht1632_putchar( 1, 8, (totalHours < 100) ? ' ' : '0' + (totalHours / 100), ORANGE);
	ht1632_putchar( 7, 8, (totalHours < 10 ) ? ' ' : '0' + ((totalHours / 10) % 10), ORANGE);
	ht1632_putchar(13, 8, '0' + (totalHours % 10), ORANGE);
	ht1632_plot(19, 10, GREEN);
	ht1632_plot(19, 12, GREEN);
	ht1632_putchar(21, 8, '0' + (totalMinutes / 10), ORANGE);
	ht1632_putchar(27, 8, '0' + (totalMinutes % 10), ORANGE);
}

//*********************************************************************************************************
// Read and write "Time Clock record" ;
void CAppTimeClock::getTimeClockdetails(int8_t project, byte* tClockState, unsigned int* inTime, unsigned int* totalTime)
{
	if ((project < 6) && (project > 0))
	{
		byte i = (project - 1) * (tClok1TotalHoursB1Loc - tClok1StateLoc + 1);
		uint16_t t;
		*tClockState = wiseClock.readUserSetting(tClok1StateLoc + i);
		t = ((uint16_t)wiseClock.readUserSetting(tClok1StartHiLoc + i) << 8) | (uint16_t)wiseClock.readUserSetting(tClok1StartLoLoc + i);
		*inTime = t;
		t = ((uint16_t)wiseClock.readUserSetting(tClok1TotalHoursB2Loc + i) << 8) | (uint16_t)wiseClock.readUserSetting(tClok1TotalHoursB1Loc + i); 
		*totalTime = t;
	}
}

//*********************************************************************************************************
void CAppTimeClock::saveTimeClockdetails(int8_t project, byte tClockState, unsigned int inTime, unsigned int totalTime)
{
	if ((project < 6) && (project > 0))
	{
		byte i = (project - 1) * (tClok1TotalHoursB1Loc - tClok1StateLoc + 1);
		wiseClock.saveUserSetting(tClok1StateLoc + i, tClockState);
		wiseClock.saveUserSetting(tClok1StartHiLoc + i, (inTime >> 8) & 0xFF);
		wiseClock.saveUserSetting(tClok1StartLoLoc + i, (inTime) & 0xFF);
		wiseClock.saveUserSetting(tClok1TotalHoursB2Loc + i, (totalTime >> 8) & 0xFF);
		wiseClock.saveUserSetting(tClok1TotalHoursB1Loc + i, totalTime & 0xFF);
	}
}

//*********************************************************************************************************
// Read and write current Time Clock Project number;
byte CAppTimeClock::getCurrentTCProject()
{
	return byte (wiseClock.readUserSetting(tClokCurrentProjectLoc));
}

//*********************************************************************************************************
void CAppTimeClock::saveCurrentTCProject()
{
	wiseClock.saveUserSetting(tClokCurrentProjectLoc, tcProject); 
}

CAppTimeClock appTimeClock;


#endif
