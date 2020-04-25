/*
 *********************************************************************************************************
 * AlarmClock.cpp
 *
 * Apr/10 by FlorinC (http://timewitharduino.blogspot.com/)
 *   Copyrighted and distributed under the terms of the Berkeley license
 *   (copy freely, but include this notice of original authors.)
 *
 * Other contributors:
 *   - Mark Sproul <MLS> msproul _at_ jove.rutgers.edu
 *
 *********************************************************************************************************
 */

//*********************************************************************************************************
//*	Edit History, started April, 2010
//*	please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//* Apr 15/10 (fc) created class, by restructuring Wise4Sure.pde;
//* Oct 10/10 (rp) changed checkAlarm(), if APP_BIG running then do not return to APP_QUOTE;
//* Feb 05/11 (fc) adding support for setting the date;
//* Jun 20/11 (rp) added chime, DST and setDateOnly();
//* Oct 15/11 (rp) added tests for low / high temperature and saving of power up time;
//* Mar 25/12 (rp) added animations every quarter and log of temperature
//* Apr 12/12 (mcm) fix DST for hours 0 and 23
//* May 05/12 (mcm) use int8_t or int16_t instead of int where appropriate
//* Jan 26/13 (mcm) prevent timeButton wrap
//* Aug 14/13 (mcm) new get12Hour() to replace common code in other modules
//*
//*********************************************************************************************************


#include "AlarmClock.h"
#include "HT1632.h"
#include "DS3231.h"
#include "TimeUtils.h"
#include "WiseClock.h"
#include "Sound.h"
#include "Buttons.h"
#include "AppTmp.h"


#ifdef WANT_ANIMATIONS
#include "AppAnimation.h"
#endif


//*********************************************************************************************************
// constructor
//
CAlarmClock::CAlarmClock()
{
  isAlarmSetting	=	false;
  isTimeSetting		=	false;
  isDateSetting		=	false;

  // true when the alarm is sounding;
  // alarm is stopped from remote control or, less practical, by pressing any button;
  isAlarmSoundOn	=	false;

  // shows if the alarm is enabled or not;
  isAlarmEnabled	=	false;

  // it could happen that the alarm is triggered and stopped in the same minute;
  // this boolean prevents the alarm to start again after it was stopped;
  // (mb)
  isAlarmTriggerEnabled	=	true;
  isChimeTriggerEnabled	=	true;

  // used for stopping the alarm sound after snoozeMins;
  timeAlarmSoundStarted	=	0;

  is24hourModeEnabled	=	true;		//*	12/24 hour mode
  
  alarmHour		=	DEFAULT_ALARM_HOUR;
  alarmMin 		=	DEFAULT_ALARM_MIN;
  
  high24Temp		=	-800;		// force setting of high / low temperature  4 x 200 degrees;
  low24Temp		=	+800;
}

//*********************************************************************************************************
void	CAlarmClock::initPrevious()
{
  previousMinute = minute;
  previousHour   = hour;
  previousDay	 = day;
  previousMonth  = month;
  previousYear   = year;
}

//*********************************************************************************************************
void	CAlarmClock::checkAlarm()
{
	int16_t temp;
#ifdef WANT_EVENT_LOG
	int16_t iTemp, dTemp, fTemp, cTemp;
#endif
	if (minute != previousMinute)				
	{
		previousMinute = minute;

		if (wiseClock.isChimeEnabled)
		{
			if (minute == 0)
			{	
				if (isChimeTriggerEnabled)
				{
					soundChimeLong();
					isChimeTriggerEnabled = false;
				}
			}
			else if (minute == 30)
			{
				if (isChimeTriggerEnabled)
				{
					soundChimeShort();
					isChimeTriggerEnabled = false;
				}
			}
			else
			{
				isChimeTriggerEnabled = true;
			}
		}
#ifdef WANT_ANIMATIONS
		if ((wiseClock.crtApp == APP_BIG) && (appAnimation.isAnimEnabled))
		{
			if ((minute % 15) == 0)
			{
				appAnimation.showQuarter(minute);
			}
		}
#endif

		temp = appTemperature.getTemperature();

		if (temp > high24Temp)
		{
			high24Temp = temp;
			high24Hour = hour;
			high24Minute = minute;
		}	
		if (temp < low24Temp)
		{
			low24Temp = temp;
			low24Hour = hour;
			low24Minute = minute;
		}
		
		RTC.saveAl2Min(minute);					// In order to detect a Power Down situation we				
									// save current minute every 60 seconds in the clock chip, cannot save in EEPROM;
									// as EEPROM is limited to 100,000 write cycles.
									// At Power Up we read this last power up time from the clock chip.
		if (hour != previousHour)
		{
			RTC.saveAl2Hour(hour);				// save current hour in the clock chip
			previousHour = hour;

#ifdef WANT_EVENT_LOG
			wiseClock.splitTemp(temp, &iTemp, &dTemp, true);
			cTemp = (iTemp * 10) + dTemp;			// log 25.5 C as 255;
			wiseClock.splitTemp(temp, &iTemp, &dTemp, false);
			fTemp = (iTemp * 10) + dTemp;

			wiseClock.updateLogFile("TE", hour, minute, day, month, year - 2000, fTemp, cTemp);
#endif

			if (day != previousDay)
			{	
				int16_t allHighTemp, allLowTemp;
				int8_t allHighYear, allHighMonth, allHighDay, allLowYear, allLowMonth, allLowDay;
				wiseClock.saveLastPowerUpDate(day, month, year - 2000);		// Save current day, month, year every day in EEPROM;

				wiseClock.getAllHighTemperature(&allHighTemp, &allHighYear, &allHighMonth, &allHighDay);
			
				if (high24Temp > allHighTemp)
					wiseClock.saveAllHighTemperature(high24Temp, previousYear - 2000, previousMonth, previousDay);

				wiseClock.getAllLowTemperature(&allLowTemp, &allLowYear, &allLowMonth, &allLowDay);
				
				if (low24Temp < allLowTemp)
					wiseClock.saveAllLowTemperature(low24Temp, previousYear - 2000, previousMonth, previousDay);

				high24Temp = -800;				// force setting of high / low temperature for next 24 hour period;
				low24Temp  = +800;				// 800 = 4 x 200 degrees;

				//
				// The timeBtn values are checked and updated when a button is pressed.
				// If a button hasn't been pressed in ~25 days, the time delta goes negative.
				// When that happens button presses are not recognized until the delta goes
				// positive again, ~25 days later.
				// Here we look for large deltas and reset timeBtn values when they get close
				// to wrapping.
				// We only have to do this once a day, which is why the code is here and not
				// in the button-checking loop.
				//
				unsigned long ms = millis() - 134217728;
				if ((long)(ms - timeBtnMenu) > 0) timeBtnMenu = ms;
				if ((long)(ms - timeBtnSet ) > 0) timeBtnSet  = ms;
				if ((long)(ms - timeBtnPlus) > 0) timeBtnPlus = ms;

				if (year != previousYear)			// reset all time high at beginning of year;
				{
					wiseClock.saveAllHighTemperature(high24Temp, previousYear - 2000, 1, 1);
					wiseClock.saveAllLowTemperature(  low24Temp, previousYear - 2000, 1, 1);
				}	
				previousDay = day;
				previousMonth = month;
				previousYear = year;
			}	
		}	
	}
	
	// alarm is stopped by pressing any button or through remote control;
	if (isAlarmSoundOn && ((long)(millis() - (timeAlarmSoundStarted + 60000)) >= 0))
	{
		isAlarmSoundOn		=	false;
	}
	if (isAlarmTriggerEnabled && isAlarmEnabled && (hour == alarmHour) && (minute == alarmMin))
	{
		isAlarmSoundOn		=	true;
		isAlarmTriggerEnabled	=	false;
		timeAlarmSoundStarted	=	millis();
		soundAlarm();
	}
	else if ((hour != alarmHour) || (minute != alarmMin))
	{
		isAlarmTriggerEnabled	=	true;
	}
}


//*********************************************************************************************************
void CAlarmClock::setTime(int8_t hh, int8_t mm, int8_t ss, int16_t year, int8_t month, int8_t day, int8_t dow)
{
	RTC.stop();
	RTC.set(DS3231_SEC,	ss);
	RTC.set(DS3231_MIN,	mm);
	RTC.set(DS3231_HR,	hh);
	RTC.set(DS3231_DOW,	dow);
	RTC.set(DS3231_DATE,	day);
	RTC.set(DS3231_MTH,	month);
	RTC.set(DS3231_YR,	year);
	RTC.start();
}

//*********************************************************************************************************
void CAlarmClock::setDateOnly()
{
	RTC.stop();
	RTC.set(DS3231_DOW,  dow);
	RTC.set(DS3231_DATE, day);
	RTC.set(DS3231_MTH,  month);
	RTC.set(DS3231_YR,   year);
	RTC.saveDateOnly();
	RTC.start();
}


//*********************************************************************************************************
void CAlarmClock::getTimeFromRTC()
{
	// (fc, Feb 5/2011) don't read RTC during time/date setting;
	//------------------------------------------------------------------------------
	// REM: this is a potential bug: since the minute is not always refreshed,
	//      alarm may not go off while playing with the menus;
	//------------------------------------------------------------------------------
	if (isTimeSetting || isDateSetting || isDaySetting)
		return;

	int16_t	rtc[7];

	RTC.get(rtc, true);

	second	=	rtc[0];
	minute	=	rtc[1];
	hour	=	rtc[2];
	dow	=	rtc[3];
	day	=	rtc[4];
	month	=	rtc[5];
	year	=	rtc[6];
}


//*********************************************************************************************************
void CAlarmClock::incrementTimeSetting(boolean isSettingHours)
{
		if (isSettingHours)
		{
			hour++;
			if (hour > 23)	hour	=	0;
		}
		else
		{
			minute++;
			if (minute > 59)	minute	=	0;
		}

		setTime(hour, minute, 0, year, month, day, dow);
}


//*********************************************************************************************************
void CAlarmClock::incrementAlarmSetting(boolean isSettingHours)
{
		if (isSettingHours)
		{
			alarmHour++;
			if (alarmHour > 23)
			{
				alarmHour	=	0;
			}
		}
		else
		{
			alarmMin++;
			if (alarmMin > 59)
			{
				alarmMin	=	0;
			}
		}
}


//*********************************************************************************************************
void CAlarmClock::incrementYear()
{
	year++;
	// since there is no decrement button, give user a chance to start from 0;
	// therefore year can be set beyong 2050 through the buttons :)
	if (year > 2050)
	    year = 2000;

}


//*********************************************************************************************************
void CAlarmClock::incrementMonth()
{
	month++;
	if (month > 12)
	    month = 1;
}


//*********************************************************************************************************
void CAlarmClock::incrementDay()
{
	day++;
	if (day > daysInMonth[month-1])
	    day = 1;
}


//*********************************************************************************************************
void CAlarmClock::saveTimeAndDate()
{
	setTime(hour, minute, 0, year, month, day, dow);
}


//*********************************************************************************************************
void CAlarmClock::incrementDOW()
{
	dow++;
	if (dow > 7)
	    dow = 1;
}


//*********************************************************************************************************
void CAlarmClock::incDSTHour()
{
	getTimeFromRTC();
	++hour;
	if (hour > 23) hour -= 24;
	setTime(hour, minute, second, year, month, day, dow);
}


//*********************************************************************************************************
void CAlarmClock::decDSTHour()
{
	getTimeFromRTC();
	--hour;
	if (hour < 0) hour += 24;
	setTime(hour, minute, second, year, month, day, dow);
}


//*********************************************************************************************************
int8_t CAlarmClock::get12Hour()
{
	int8_t hourAm = hour;
	if (!is24hourModeEnabled) {
		if (hourAm > 12)
			hourAm -= 12;
		else if (hourAm == 0) hourAm = 12;
	}
	return(hourAm);
}

CAlarmClock alarmClock;

