// AppUtc.cpp

// (fc, Jul 5/10) display UTC time on the top line (instead of quotes);
//*********************************************************************************************************
//*     Edit History, started April, 2012
//*     please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//*     Apr  26,        2012    (mcm) Code moved from wiseClock3.cpp to here
//*     Apr  26,        2012    (mcm) moved analog clock position arrays to PROGMEM
//*     May  02,        2012    (mcm) use #define for scroll prefix
//*	May  05,	2012	(mcm) use int8_t or int16_t instead of int where appropriate
//*	May  28,	2012	(mcm) restructure to eliminate delay() calls
//*	Jun  25,	2012	(mcm) changes to better support multiple displays
//*********************************************************************************************************
//
#include "UserConf.h"
#ifdef WANT_APP_UTC

#include "WiseClock.h"
#include "HT1632.h"
#include "AlarmClock.h"
#include "AppUtc.h"
#include <avr/pgmspace.h>


void CAppUtc::init(byte st /*=0*/)
{
  // (fc, Sep 15, 2013) moved from WiseClock::startApp();
	if (st)
	{
		wiseClock.buttonsInUse 	= 	SET_BUTTON + PLUS_BUTTON;
		wiseClock.plusButtonCnt	= 	wiseClock.utcDiff;
		wiseClock.setButtonCnt 	= 	0;
		utcState = 1;
	}
	else
	{
		utcState = 3;
	}

}


int16_t CAppUtc::run()
{
	if (utcState == 1)
	{
		if (wiseClock.plusButtonCnt > 12)						// runs from 0 --> 12 --> -12 --> -0
			wiseClock.plusButtonCnt = -12;

		ht1632_putchar( 1, 8, ' ', BLACK);
		ht1632_putchar(25, 8, ' ', BLACK);
	
		ht1632_putchar( 7, 8, (wiseClock.plusButtonCnt < 0) ? '-' : '+', ORANGE);
		ht1632_putchar(13, 8, (abs(wiseClock.plusButtonCnt) < 10) ? ('0' + abs(wiseClock.plusButtonCnt)) : ('0' + abs(wiseClock.plusButtonCnt / 10)), ORANGE);
		ht1632_putchar(19, 8, (abs(wiseClock.plusButtonCnt) < 10) ? ' ' : ('0' + abs(wiseClock.plusButtonCnt) % 10), ORANGE);
		
		if (wiseClock.setButtonCnt == 0)						// wait till Set button pressed;
			return(50);
			
		wiseClock.setButtonCnt = 0;
		wiseClock.utcDiff = wiseClock.plusButtonCnt;
		saveUtcDiff();
		utcState = 2;
		wiseClock.plusButtonCnt = 0;
	}
	
	if (utcState == 2)
	{
		if (wiseClock.plusButtonCnt > 1)						
			wiseClock.plusButtonCnt = 0;
			
		if (wiseClock.plusButtonCnt == 0)						
			displayStaticLine_P(PSTR("Graph"), 0, GREEN);
		else
			displayStaticLine_P(PSTR("Text "), 0, GREEN);

		if (wiseClock.setButtonCnt == 0)						// wait till Set button pressed;
			return(50);

		utcState = wiseClock.plusButtonCnt + 3;
		wiseClock.setButtonCnt = 0;							// wait till Set button pressed;
		wiseClock.buttonsInUse = 0;
		clearDisplay();
		wiseClock.crtPos = 9999;	// force a fresh start;
	}

	if (utcState == 4)
	{
		if (wiseClock.isLineInProgress())
		{
			wiseClock.crtColor = GREEN;
			if (wiseClock.largeTextFont == false)
				wiseClock.displayTime(8, false);
		}
		else
		{
			fetchUtcTime();  						// fills the text buffer with the utc time;
		}
	} 

	else if (utcState == 3)
	{
#ifdef MULTI_DISPLAY_SCROLL
		if (wiseClock.utcDiff == 0 && !wiseClock.DSTactive)
			showAnaClock(24, 0, false, true);
		else
		{
			showAnaClock(12, 0, false, false);
			showAnaClock(37, wiseClock.utcDiff, true, false);
		}
#else
		if (wiseClock.utcDiff == 0 && !wiseClock.DSTactive)
			showAnaClock(8, 0, false, true);
		else
		{
			showAnaClock( 0, 0, false, false);
			showAnaClock(17, wiseClock.utcDiff, true, false);
		}
#endif
	}
	return(100);
}	

//*********************************************************************************************************
// Display time as analog clock;
//

// LED Positions for analog clock;
const byte clk[12] PROGMEM = { 0x72, 0xa3, 0xc5, 0xd8, 0xcb, 0xad, 0x7e, 0x4d, 0x2b, 0x18, 0x25, 0x43 };
const byte hrs[26] PROGMEM = { 0x64, 0x73, 0x84, 0x94, 0xa5, 0xb6, 0xb7, 0xc8, 0xb9, 0xba, 0xab, 0x9c, 0x8c, 0x7d, 0x6c, 0x5c, 0x4b, 0x3a, 0x39, 0x28, 0x37, 0x36, 0x45, 0x54, 0x64, 0x73 };
const byte mns[26] PROGMEM = { 0x51, 0x71, 0x91, 0xb2, 0xc3, 0xd4, 0xe6, 0xe8, 0xea, 0xdc, 0xcd, 0xbe, 0x9f, 0x7f, 0x5f, 0x3e, 0x2d, 0x1c, 0x0a, 0x08, 0x06, 0x14, 0x23, 0x32, 0x51, 0x71 };


void CAppUtc::showAnaClock(coord_t o, int8_t u, boolean utc, boolean showSecs)
{
	int8_t hh, mm, ss;
	uint8_t v;

	for (ss = 0; ss < 12; ss++) {
		v = pgm_read_byte_near(&clk[ss]);
		ht1632_plot((v >> 4) + o, v & 0xf, GREEN);
	}
	
//	if (alarmClock.isAlarmEnabled)
//		ht1632_plot(15, 0, wiseClock.getColor());			// show alarm is set;
	
	hh = alarmClock.hour;
	if (utc) {
		hh -= u;
		hh += 24;
		if (wiseClock.DSTactive) hh += 23;
		hh %= 24;
	}
	mm = alarmClock.minute;
	
	if (hh > 11)
		hh = hh - 12;
	hh = hh << 1;

	if (mm > 47)
		hh = hh + 2;
	else
		if (mm > 12)
			hh = hh + 1;
			
	v = pgm_read_byte_near(&hrs[hh]);
	ht1632_plot((v >> 4) + o, v & 0xf, BLACK);	// clear previous hour;
	v = pgm_read_byte_near(&hrs[hh+1]);
	ht1632_plot((v >> 4) + o, v & 0xf, ORANGE);	// hours;

	mm = (((mm + 1) << 1) / 5);

	v = pgm_read_byte_near(&mns[mm]);
	ht1632_plot((v >> 4) + o, v & 0xf, BLACK);	// clear previous minute;
	v = pgm_read_byte_near(&mns[mm+1]);
	ht1632_plot((v >> 4) + o, v & 0xf, RED);	// minute;

	ss = alarmClock.second;
	if (ss & 1)
		ht1632_plot(7 + o,8, RED);									// alternate center dot red / orange;
	else	
		ht1632_plot(7 + o,8, ORANGE);
		
	if (showSecs)
	{
		ss = (((ss + 1) << 1) / 5);
		if (ss != (mm + 1))
		{
			v = pgm_read_byte_near(&mns[ss]);
			ht1632_plot((v >> 4) + o, v & 0xf, BLACK);	// clear previous second;
		}
		if (ss != mm)
		{
			v = pgm_read_byte_near(&mns[ss+1]);
			ht1632_plot((v >> 4) + o, v & 0xf, GREEN);	// seconds
		}
	}		
}

//*********************************************************************************************************
void CAppUtc::fetchUtcTime()
{
	int8_t utcHour = (alarmClock.hour - wiseClock.utcDiff) + 24;

	if (wiseClock.DSTactive) utcHour += 23;
	utcHour %= 24;
	memset(wiseClock.crtBuffer, ' ', BLANK_PREFIX);
	sprintf_P(&wiseClock.crtBuffer[BLANK_PREFIX], PSTR("UTC %02d:%02d  "), utcHour, alarmClock.minute);

#ifdef _DEBUG_
	Serial.println(wiseClock.crtBuffer);
#endif

	wiseClock.resetCrtPos();
}


//*********************************************************************************************************
// Read and write UTC difference;
int8_t CAppUtc::getUtcDiff()
{
	return wiseClock.readUserSetting( utcDiffLoc ) - 24;
}

//*********************************************************************************************************
void CAppUtc::saveUtcDiff()
{
	wiseClock.saveUserSetting( utcDiffLoc, wiseClock.utcDiff + 24); 
#ifdef _DEBUG_
	Serial.print("Saved UTC difference: ");
	Serial.println(utcDiff);
#endif
}
CAppUtc appUtc;

#endif
