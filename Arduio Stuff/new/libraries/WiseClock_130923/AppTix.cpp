// AppTix.cpp
// show time HH:MM as in TIX Clock

//*********************************************************************************************************
//*	Edit History, started Oct, 2011
//*	please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//*	Oct  15,	2011	(rp) first entry;
//*	May  28,	2012	(mcm) restructure to eliminate delay() calls
//*	Jun  03,	2012	(mcm) add racing red seconds dot, sync update to seconds change
//*	Jun  25,	2012	(mcm) changes to better support multiple displays
//*	Aug  14,	2013	(mcm) use get12Hour() instead of calculating 12-hour mode
//*
//*********************************************************************************************************


#include "UserConf.h"
#ifdef WANT_APP_TIX

#include "AppTix.h"
#include "HT1632.h"
#include "AlarmClock.h"
#include "WiseClock.h"
									// there are 27 squares in 4 groups, 3 + 9 + 6 + 9;
byte tixColor[27];							// contains color for each square;
#ifdef MULTI_DISPLAY_SCROLL
const byte tixXPos[9] PROGMEM	= {8,  14,19,24,  31,36,  42,47,52};		// contains horizontal position for each column;
#define X_OFFSET 16
#else
const byte tixXPos[9] PROGMEM	= {0,  3,7,11,  14,18,  21,25,29};		// contains horizontal position for each column;
#define X_OFFSET 0
#endif
byte tixYPos[3] 	= {1,6,11};					// contains vertical postion for each row;
const byte tixText[13] PROGMEM = {20,32,33,34,44,60,61,62,88,90,101,112,114};	// contains square numbers for TIX start up text;


//*********************************************************************************************************
void CAppTix::init(boolean start /*=0*/)
{
	wiseClock.plusButtonCnt = 0;
	wiseClock.setButtonCnt = 0;
	startupAppTix = start;
	if (start)			// start by asking for update interval
	{
		wiseClock.buttonsInUse  =	SET_BUTTON + PLUS_BUTTON;
	}
	else
	{
		wiseClock.buttonsInUse  =	0;
		tixSpeed = 4;
		clearTixDisplay = true;
	}
	lastSecond = 60;
}


//*********************************************************************************************************
int16_t CAppTix::run()
{
	byte i, j, k, t;
	
	if (startupAppTix == true)
	{
		if (wiseClock.plusButtonCnt > 2)						// Update display every 1, 4, 60 seconds;
			wiseClock.plusButtonCnt = 0;

		switch(wiseClock.plusButtonCnt)
		{
		    case 0:
			displayStaticLine_P(PSTR("1 Sec"), 0, GREEN);
			tixSpeed = 1;
			break;
		    case 1:
			displayStaticLine_P(PSTR("4 Sec"), 0, GREEN);
			tixSpeed = 4;
			break;
		    case 2:
			displayStaticLine_P(PSTR("60Sec"), 0, GREEN);
			tixSpeed = 60;
			break;
		}

		wiseClock.displayTime(8, false);
		if (wiseClock.setButtonCnt == 0)						// wait till Set button pressed;
			return(100);
		
		wiseClock.setButtonCnt = 0;
		wiseClock.plusButtonCnt = 0;
		wiseClock.buttonsInUse = 0;
	
		clearDisplay();
	
		for (i = 0; i < 13; i++)
		{
			for (j = 0; j < 3; j++)
			{
				for (k = 0; k < 4; k++)
				{
					t = pgm_read_byte_near(&tixText[i]);
					ht1632_plot((t >> 2) + j + X_OFFSET, tixYPos[t & 3] + k, RED); 	// display TIX text;
				}
			}
		}
		startupAppTix = false;
		clearTixDisplay = true;
		return(2000);
	}

	if (alarmClock.second != lastSecond)
	{
		if (lastSecond < 30)
			ht1632_plot(lastSecond + X_OFFSET + 1, 0, BLACK);
		else
			ht1632_plot(60 + X_OFFSET - lastSecond, 15, BLACK);
		lastSecond = alarmClock.second;
		if (lastSecond < 30)
			ht1632_plot(lastSecond + X_OFFSET + 1, 0, RED);
		else
			ht1632_plot(60 + X_OFFSET - lastSecond, 15, RED);

		if (clearTixDisplay || ((lastSecond % tixSpeed) == 0))
		{
			if (clearTixDisplay)
			{
				clearDisplay();
				clearTixDisplay = false;
			}
			byte hour = alarmClock.get12Hour();
			byte minute = alarmClock.minute;

			for (i = 0; i < 27; i++)
				tixColor[i] = BLACK;				// set all 27 squares to black;
			setTixColor(hour   / 10,  0,  3, GREEN);		// set squares for first  digit to correct color;
			setTixColor(hour   % 10,  3, 12, ORANGE);		// set squares for second digit to correct color;
			setTixColor(minute / 10, 12, 18, GREEN);		// set squares for third  digit to correct color;
			setTixColor(minute % 10, 18, 27, ORANGE);		// set squares for fourth digit to correct color;
	
			for (i = 0; i < 27; i++)
			{
#ifdef MULTI_DISPLAY_SCROLL
				for (j = 0; j < 4; j++)
#else
				for (j = 0; j < 3; j++)
#endif
				{
					for (k = 0; k < 4; k++)
					{
						t = pgm_read_byte_near(&tixXPos[i / 3]);
						ht1632_plot(t + j, tixYPos[i % 3] + k, tixColor[i]); 	// display all 27 squares
					}
				}
			}
		}
	}
	return(100);
}

	
//*********************************************************************************************************
void CAppTix::setTixColor(byte number, byte first, byte last, byte color)
{
	byte j;
	
	while (number > 0)
	{
		j = random(first, last);
		if (tixColor[j] == BLACK)
		{
			tixColor[j] = color;
			number--;
		}
	}
}

CAppTix appTix;
#endif
