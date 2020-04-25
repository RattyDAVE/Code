// AppStopWatch.cpp

//*********************************************************************************************************
//*	Edit History, started Oct, 2011
//*	please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//*	Oct  15,	2011	(rp) first entry;
//*	May  28,	2012	(mcm) restructure to eliminate delay() calls
//*	Jun  25,	2012	(mcm) new args to displayTime()
//*	Sep  21,	2013	(fc) on dual display use either big font (if selected) or regular font;
//*				(fc) added "conventional" chrono mode as well (see comment below);
//*
//*********************************************************************************************************

#include "UserConf.h"
#ifdef WANT_APP_STOPW

#include "AppStopWatch.h"
#include "HT1632.h"
#include "WiseClock.h"
#include "AlarmClock.h"
#include "Sound.h"

#ifdef MULTI_DISPLAY_SCROLL
 #define X_OFFSET 10
#else
 #define X_OFFSET 0
#endif


#define BIG_CHRONO_FONT  2


// (fc, Sep 21, 2013) the only mode was "Rattrapante": pressing Set button shows the amount of time passed from the moment it was first started;
// the conventional chronometer mode counts only the amount of time between pushes of the Set button;

enum
{
  CONVENTIONAL = 'C',
  RATTRAPANTE  = 'R'
};



void CAppStopWatch::init(byte mode /*=0*/)
{
	clearDisplay();
	wiseClock.buttonsInUse = SET_BUTTON + PLUS_BUTTON;
	wiseClock.plusButtonCnt = 0;
	wiseClock.setButtonCnt = 0;
	isSetup = true;

	hour = 0;
	minute = 0;
	second = 0;
	prevSecond = 0;

	chronoMode = getChronoMode();
	if (chronoMode != CONVENTIONAL && chronoMode != RATTRAPANTE)
	{
		// set default chrono mode to conventional...
		chronoMode = CONVENTIONAL;
		// ...and save to eeprom;
		saveChronoMode();
	}
}


int16_t CAppStopWatch::run()
{
	long currentTime, diffTime;

	if (isSetup)
	{
#ifdef MULTI_DISPLAY_SCROLL
		if (wiseClock.largeTextFont == true)
		{
			ht1632_putBigDigit( 4, 2, 0, BIG_CHRONO_FONT, GREEN, 6);
			ht1632_putBigDigit(11, 2, 0, BIG_CHRONO_FONT, GREEN, 6);
			ht1632_putBigDigit(24, 2, 0, BIG_CHRONO_FONT, GREEN, 6);
			ht1632_putBigDigit(31, 2, 0, BIG_CHRONO_FONT, GREEN, 6);
			ht1632_putBigDigit(44, 2, 0, BIG_CHRONO_FONT, GREEN, 6);
			ht1632_putBigDigit(51, 2, 0, BIG_CHRONO_FONT, GREEN, 6);
			ht1632_putchar(59, 7, '0', GREEN);

		}
		else
		{
			ht1632_putString  (-1 + X_OFFSET, 0, "00:00:00", GREEN);
			ht1632_putTinyChar(46 + X_OFFSET, 1, '0', GREEN);
		}

		// indicate the mode (CONVENTIONAL or RATTRAPANTE);
		ht1632_putTinyChar(-1, 1, chronoMode, GREEN);

#else	// single display
		ht1632_putTinyChar(-1 + X_OFFSET, 1, '0', GREEN);
		ht1632_putTinyChar( 3 + X_OFFSET, 1, '0', GREEN);

		ht1632_plot(8 + X_OFFSET, 3, GREEN);
		ht1632_plot(8 + X_OFFSET, 5, GREEN);

		ht1632_putTinyChar( 9 + X_OFFSET, 1, '0', GREEN);
		ht1632_putTinyChar(13 + X_OFFSET, 1, '0', GREEN);

		ht1632_plot(18 + X_OFFSET, 3, GREEN);
		ht1632_plot(18 + X_OFFSET, 5, GREEN);

		ht1632_putTinyChar(19 + X_OFFSET, 1, '0', GREEN);
		ht1632_putTinyChar(23 + X_OFFSET, 1, '0', GREEN);
		ht1632_putTinyChar(28 + X_OFFSET, 1, '0', GREEN);

		// indicate the mode (CONVENTIONAL or RATTRAPANTE);
		ht1632_plot(0, 0, chronoMode == CONVENTIONAL ? BLACK : GREEN);
#endif
		if (wiseClock.largeTextFont == false)
		{
			wiseClock.displayTime(8, false);
		}

		if (wiseClock.setButtonCnt > 0)				// if Set button is pressed, then start counting;
		{
			startTime = (long)((alarmClock.hour * 3600l) + (alarmClock.minute * 60) + alarmClock.second);
			wiseClock.buttonsInUse = SET_BUTTON;	// the Plus key can now be used to change the brightness;
			wiseClock.setButtonCnt = 0;
			isSetup = false;
			isStopped = false;
			tensec = 0;
			prevSecond = 0;

			recordStartAppTime();	 	// Start new cycle
		}

		// Plus button changes the chrono mode between CONVENTIONAL and RATTRAPANTE;
		if (wiseClock.plusButtonCnt > 0)
		{
			wiseClock.plusButtonCnt = 0;

			if (chronoMode == CONVENTIONAL)
				chronoMode = RATTRAPANTE;
			else
				chronoMode = CONVENTIONAL;

			saveChronoMode();

			// indicate the mode (CONVENTIONAL or RATTRAPANTE);
#ifdef MULTI_DISPLAY_SCROLL
			ht1632_putTinyChar(-1, 1, chronoMode, GREEN);
#else
			ht1632_plot(0, 0, chronoMode == CONVENTIONAL ? BLACK : GREEN);
#endif
		}
	}
	else
	{
		if (!isStopped)
		{
			if (chronoMode == RATTRAPANTE)
			{
				currentTime = (long)((alarmClock.hour * 3600l) + (alarmClock.minute * 60) + alarmClock.second);
				if (currentTime < startTime)
					currentTime = currentTime + long(24l * 3600l);		// correction if we go from 23:59 - > 00:00;
				diffTime = currentTime - startTime;

				// amount of time from the beginning;
				hour = (long)(diffTime / 3600l);
				diffTime = (long)(diffTime % 3600l);
				minute = diffTime / 60;
				second = diffTime % 60;
			}

			if (hasAppTimePassed(90))          		// Has 100msec passed ? (= 90 msec + processing time)
			{
				recordStartAppTime();          		// Start new cycle

				if (prevSecond != alarmClock.second)		// if new second then make hundred second zero;
				{
					prevSecond = alarmClock.second;
					tensec = 0xff;				// reset ten milli second counter to zero

					if (chronoMode == CONVENTIONAL)
						AddOneSecond();
				}	

				if (++tensec > 9)
					tensec = 0;
			}
		}

#ifdef MULTI_DISPLAY_SCROLL
		if (wiseClock.largeTextFont == true)
		{
			ht1632_putBigDigit( 4, 2, hour / 10, BIG_CHRONO_FONT, ORANGE, 6);
			ht1632_putBigDigit(11, 2, hour % 10, BIG_CHRONO_FONT, ORANGE, 6);
			ht1632_putBigDigit(24, 2, minute/10, BIG_CHRONO_FONT, ORANGE, 6);
			ht1632_putBigDigit(31, 2, minute%10, BIG_CHRONO_FONT, ORANGE, 6);
			ht1632_putBigDigit(44, 2, second/10, BIG_CHRONO_FONT, ORANGE, 6);
			ht1632_putBigDigit(51, 2, second%10, BIG_CHRONO_FONT, ORANGE, 6);
			ht1632_putchar(59, 7, '0' + tensec, GREEN);

			// separating dots;
			ht1632_plot(20, 7, ORANGE);
			ht1632_plot(21, 7, ORANGE);
			ht1632_plot(20, 8, ORANGE);
			ht1632_plot(21, 8, ORANGE);

			ht1632_plot(40, 7, ORANGE);
			ht1632_plot(41, 7, ORANGE);
			ht1632_plot(40, 8, ORANGE);
			ht1632_plot(41, 8, ORANGE);
		}
		else
		{
			char strChrono[10] = "";
			sprintf_P(strChrono, PSTR("%02d:%02d:%02d"), hour, minute, second);

			ht1632_putString  (-1 + X_OFFSET, 0, strChrono, ORANGE);
			ht1632_putTinyChar(46 + X_OFFSET, 1, '0' + tensec, GREEN);
		}

#else
	// single display
		ht1632_putTinyChar(-1 + X_OFFSET, 1, '0' + hour / 10, ORANGE);
		ht1632_putTinyChar( 3 + X_OFFSET, 1, '0' + hour % 10, ORANGE);

		ht1632_plot(8 + X_OFFSET, 3, GREEN);
		ht1632_plot(8 + X_OFFSET, 5, GREEN);

		ht1632_putTinyChar( 9 + X_OFFSET, 1, '0' + minute / 10, ORANGE);
		ht1632_putTinyChar(13 + X_OFFSET, 1, '0' + minute % 10, ORANGE);

		ht1632_plot(18 + X_OFFSET, 3, GREEN);
		ht1632_plot(18 + X_OFFSET, 5, GREEN);

		ht1632_putTinyChar(19 + X_OFFSET, 1, '0' + second / 10, ORANGE);
		ht1632_putTinyChar(23 + X_OFFSET, 1, '0' + second % 10, ORANGE);
		ht1632_putTinyChar(28 + X_OFFSET, 1, '0' + tensec, GREEN);

#endif
		if (wiseClock.setButtonCnt > 0)
		{
			wiseClock.setButtonCnt = 0;

			// in conventional mode, this is the new start time;
			if (isStopped && chronoMode == CONVENTIONAL)
			{
				prevSecond = alarmClock.second;
			}

			isStopped = !isStopped;
		}	

		if (wiseClock.largeTextFont == false)
		{
			wiseClock.displayTime(8, false);
		}
	}
	return(20);
}


// update the conventional chrono;
//
void CAppStopWatch::AddOneSecond()
{
	second++;
	if (second > 59)
	{
		second = 0;
		minute++;
	}
	if (minute > 59)
	{
		minute = 0;
		hour++;
	}
}

//*********************************************************************************************************
// Read and write chrono mode, conventional (C) or rattrapante (R);
//
byte CAppStopWatch::getChronoMode()
{
	// returns either 'C' or 'R';
	return wiseClock.readUserSetting(chronoModeLoc);
}

void CAppStopWatch::saveChronoMode()
{
	wiseClock.saveUserSetting(chronoModeLoc, chronoMode);
}


CAppStopWatch appStopWatch;

#endif
