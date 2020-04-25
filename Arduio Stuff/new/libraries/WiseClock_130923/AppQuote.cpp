// AppQuote.cpp
 
//*********************************************************************************************************
//*	Edit History
//*	please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//*	Mar 28, 2011	(fc) created;
//*	May 02, 2012	(mcm) use #define for scroll prefix
//*	May 04, 2012	(mcm) alternate colon/dot blink for alarm indicator
//*	May 05, 2012	(mcm) use int8_t or int16_t instead of int where appropriate
//*	May 28, 2012	(mcm) restructure to eliminate delay() calls
//*	Jun 17, 2012	(mcm) convert from ifstream to SdFile
//*	Jun 25,	2012	(mcm) new args to displayTime()
//*     Aug 14, 2013	(mcm) use get12Hour() instead of calculating 12-hour mode
//*	Sep 23, 2013	(mcm) optionally randomize  quote order
//*********************************************************************************************************


#include "UserConf.h"
#ifdef WANT_APP_QUOTE

#include "AppQuote.h"
#include "HT1632.h"
#include "WiseClock.h"
#include "AlarmClock.h"

#ifdef WANT_EVENT_LOG
  #define QUOTE_FILE_LIM	12
#else
  #define QUOTE_FILE_LIM	11
#endif


//*********************************************************************************************************
void CAppQuote::init(byte mode /*=0*/)
{
	wiseClock.plusButtonCnt = getQuoteFileDigit();
	wiseClock.setButtonCnt = 0;
	wiseClock.startUpAppQuote = true;
#ifdef WANT_QUOTE_RANDOM
	doRand = wiseClock.readUserSetting(quoteRandLoc) != 0;
	rnd = 1;
#endif
	quoteState = 0;
}


//*********************************************************************************************************
int16_t CAppQuote::run()
{
	byte hour;

	if (wiseClock.startUpAppQuote == true)
	{
	    if (quoteState == 0)
	    {
		if (wiseClock.plusButtonCnt > QUOTE_FILE_LIM)				// file name runs from quot1 -> quot0;
			wiseClock.plusButtonCnt = 1;

		wiseClock.displayTime(8, false);
		if (wiseClock.plusButtonCnt == 11)
		{
			strcpy_P(wiseClock.tempBuffer, PSTR("message.txt"));
			displayStaticLine_P(PSTR(" MSG "), 0, GREEN);
		}
#ifdef WANT_EVENT_LOG
		else if (wiseClock.plusButtonCnt == 12)
		{
			strcpy_P(wiseClock.tempBuffer, PSTR("WCLOG.CSV"));
			displayStaticLine_P(PSTR(" LOG "), 0, GREEN);
		}
#endif
		else
		{
			strcpy_P(wiseClock.tempBuffer, PSTR("quot1"));
			wiseClock.tempBuffer[4] = '0' + (wiseClock.plusButtonCnt % 10);
			displayStaticLine(wiseClock.tempBuffer, 0, GREEN);
			strcat(wiseClock.tempBuffer, ".txt");
		}
		
		if (wiseClock.setButtonCnt == 0)			// wait till Set button pressed; 
			return(100);

		if (!wiseClock.openLongFile(wiseClock.tempBuffer))	// Test if file exists;
		{
			displayStaticLine_P(PSTR("Wrong"), 0, RED);
			wiseClock.setButtonCnt = 0;				// try again with a different quote file ?
			return(2000);
		}
		saveQuoteFileDigit(wiseClock.plusButtonCnt % 10);
		quoteState = 1;
#ifdef WANT_QUOTE_RANDOM
		wiseClock.plusButtonCnt = doRand;
		wiseClock.setButtonCnt = 0;
		return(100);
	    }
	    else
	    {
		if (wiseClock.plusButtonCnt > 1)
		    wiseClock.plusButtonCnt = 0;
		if (wiseClock.plusButtonCnt == 0)
		    displayStaticLine_P(PSTR(" SEQ "), 0, GREEN);
		else
		    displayStaticLine_P(PSTR(" RND "), 0, GREEN);

		if (wiseClock.setButtonCnt == 0)			// wait till Set button pressed; 
			return(100);

		doRand = wiseClock.plusButtonCnt != 0;
		wiseClock.saveUserSetting(quoteRandLoc, doRand);
#endif
	    }
	    wiseClock.setButtonCnt = 0;
	    quoteState = 0;
	    wiseClock.buttonsInUse	= 0;
	    wiseClock.startUpAppQuote = false;
	}
	
	if (wiseClock.largeTextFont == false)
		// Always display the "latest" time so that "Chime" and display are in sync;
		wiseClock.displayTime(8, false);

	if (!wiseClock.isLineInProgress())
	{	
		if (wiseClock.showExtraMessages == 0)
		{
			wiseClock.showExtraMessages = 1;
			if (wiseClock.largeTextFont == true)
			{
				byte hour = alarmClock.get12Hour();
				char separator = ':';
				if (alarmClock.isAlarmEnabled)
					separator = '.';
				memset(wiseClock.crtBuffer, ' ', BLANK_PREFIX);
				sprintf_P(&wiseClock.crtBuffer[BLANK_PREFIX], PSTR("Time = %d%c%02d "), hour, separator, alarmClock.minute);
				wiseClock.resetCrtPos();
				wiseClock.crtColor		= 	wiseClock.getColor();
				return(100);
			}
		}
		
		if (wiseClock.showExtraMessages == 1)
		{
			// insert enough spaces to look like the string starts scrolling from beyond the display;
			memset(wiseClock.crtBuffer, ' ', BLANK_PREFIX);
			wiseClock.readLineFromLongSD(&wiseClock.crtBuffer[BLANK_PREFIX], MAX_MSG_LEN - BLANK_PREFIX);  // fills the text buffer with the quote message;
			wiseClock.resetCrtPos();
			wiseClock.crtColor = GREEN;
			// (fc, Apr 15/2010) new feature: skip commented lines (starting with character #);
			if (wiseClock.isLongComment)
			{
				do {
					wiseClock.crtPos = 9999; // force reading new line;
				} while(wiseClock.isLineInProgress());
				return(0);
			}
#ifdef WANT_QUOTE_RANDOM
			if (doRand)
			{
				if (--rnd == 0)			// choose new random skip
				{
					rnd = random(1,64);
				}
				else				// skip this line
				{
					do {
						wiseClock.crtPos = 9999; // force reading new line;
					} while(wiseClock.isLineInProgress());
					return(0);		// no further processing
				}
			}
#endif
			wiseClock.showExtraMessages = 2;
		}
		else
		{
			wiseClock.showExtraMessages = 0;
			wiseClock.fetchExtraInfo();
		}	
	}
	return(100);
}

//*********************************************************************************************************
// Read and write last digit of quot?.txt file;
byte CAppQuote::getQuoteFileDigit()
{
	return byte (wiseClock.readUserSetting( quoteFileLoc ));
}

//*********************************************************************************************************
void CAppQuote::saveQuoteFileDigit(byte quoteFileDigit)
{
	wiseClock.saveUserSetting(quoteFileLoc, quoteFileDigit); 
}

CAppQuote appQuote;
#endif
