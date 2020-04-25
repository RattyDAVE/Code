// AppTimeLived.cpp

//*********************************************************************************************************
//*	Edit History, started March, 2012
//*	please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//*	Mar 14, 2012	(fc) created;
//*	Apr 14, 2012	(mcm) changed math resolution to minutes, offset to 1900 (from 1950);
//*	May 05, 2012	(mcm) use int8_t or int16_t instead of int where appropriate
//*	May 28, 2012	(mcm) restructure to eliminate delay() calls
//*
//*********************************************************************************************************


#include "UserConf.h"
#ifdef WANT_APP_LIVED

#include "AppTimeLived.h"
#include "HT1632.h"
#include "TimeUtils.h"
#include "WiseClock.h"
#include "AlarmClock.h"
#include "Sound.h"

#ifdef MULTI_DISPLAY_SCROLL
 #define X_OFFSET 16
#else
 #define X_OFFSET 0
#endif


enum				// display modes for "Time lived"; 
{
  TIME_ALL = 0,		// show years, months, days, hours, minutes and seconds;
  DAYS_ONLY,
  HOURS_ONLY,
  MINUTES_ONLY
};


void CAppTimeLived::init(byte mode /*=0*/)
{
  wiseClock.initMessageFromSDcard();
#ifdef _DEBUG_
  Serial.print("Birth year ");
  Serial.println(birthYear, DEC);
  Serial.print("Birth month ");
  Serial.println(birthMonth, DEC);
  Serial.print("Birth day ");
  Serial.println(birthDay, DEC);
  Serial.print("Birth hour ");
  Serial.println(birthHour, DEC);
  Serial.print("Birth minute ");
  Serial.println(birthMinute, DEC);
  Serial.print("Birth second ");
  Serial.println(birthSecond, DEC);
#endif

  // REM: these will be read from SD card;
/*
  birthYear = 1967;
  birthMonth = 3;
  birthDay = 12;
  birthHour = 13;
  birthMinute = 1;
  birthSecond = 0;
*/
  clearDisplay();
  modeSelected = TIME_ALL;
  wiseClock.setButtonCnt = 0;				// Set buttom used to step to the next big mode;

  birthTime = makeTimeInMinutes(birthYear, birthMonth, birthDay, birthHour, birthMinute);
#ifdef _DEBUG_
  Serial.print("Birth time in minutes ");
  Serial.println(birthTime, DEC);
#endif

  // calculate difference between current time and time and birth;
  // diffTime();
/*
  time_t t1 = makeTimeInMinutes(alarmClock.year, alarmClock.month, alarmClock.day, alarmClock.hour, alarmClock.minute);
  time_t seconds = t1 - birthTime;
*/
}


int16_t CAppTimeLived::run()
{
  wiseClock.buttonsInUse = SET_BUTTON;	// Set button is used in TimeLived mode; 

  if (wiseClock.setButtonCnt != 0)
  {
    wiseClock.setButtonCnt = 0;
    if (++modeSelected > MINUTES_ONLY)
      modeSelected = TIME_ALL;
    clearDisplay();
  }


  if (birthTime == 0)
  {
	ht1632_putTinyString(0 + X_OFFSET, 0, "No BDate", RED);
	return(250);
  }

  // update the display depending on the mode;
  switch (modeSelected)
  {
     case TIME_ALL:
       // calculate difference between current time and time and birth;
       diffTime();
       // display years, months, days on the top row;
       sprintf_P(wiseClock.tempBuffer, PSTR("%2d %2d %2d"), yearsLived, monthsLived, daysLived);
       ht1632_putTinyString(1 + X_OFFSET, 0, wiseClock.tempBuffer, ORANGE);
       // display hours, minutes, seconds on the bottom row;
       sprintf_P(wiseClock.tempBuffer, PSTR("%2d %2d %2d"), hoursLived, minutesLived, secondsLived);
       ht1632_putTinyString(1 + X_OFFSET, 8, wiseClock.tempBuffer, RED);
       break;

     case DAYS_ONLY:
       // calculate the days passed;
       ht1632_putTinyString(0 + X_OFFSET, 0, "My days", ORANGE);
       // display total numbers of days passed (max 5 digits);
       sprintf_P(wiseClock.tempBuffer, PSTR("%lu"), diffDays());
       ht1632_putTinyString(1 + X_OFFSET, 8, wiseClock.tempBuffer, RED);
       break;

     case HOURS_ONLY:
       // calculate the hours passed;
       diffHours();
       ht1632_putTinyString(0 + X_OFFSET, 0, "My hours", ORANGE);
       // display total numbers of hours passed (max 6 digits);
       sprintf_P(wiseClock.tempBuffer, PSTR("%lu"), diffHours());
       ht1632_putTinyString(1 + X_OFFSET, 8, wiseClock.tempBuffer, RED);
       break;

     case MINUTES_ONLY:
       // calculate the minutes passed;
       diffMinutes();
       ht1632_putTinyString(0 + X_OFFSET, 0, "MyMinuts", ORANGE);
       // display total numbers of minutes passed (max 8 digits);
       sprintf_P(wiseClock.tempBuffer, PSTR("%lu"), diffMinutes());
       ht1632_putTinyString(1 + X_OFFSET, 8, wiseClock.tempBuffer, RED);
       break;
  }
  return(250);
}


//***************************************************************************
void CAppTimeLived::diffTime()
{
  time_t t1 = makeTimeInMinutes(alarmClock.year, alarmClock.month, alarmClock.day, alarmClock.hour, alarmClock.minute);
  time_t minutes = t1 - birthTime;
  unsigned long leftoverMins;

  if (minutes > (4 * MINS_PER_YEAR)) {
    yearsLived   = minutes / (MINS_PER_YEAR + (MINS_PER_DAY/4));
    leftoverMins = minutes % (MINS_PER_YEAR + (MINS_PER_DAY/4));
  } else {
    yearsLived   = minutes / MINS_PER_YEAR;
    leftoverMins = minutes % MINS_PER_YEAR;
  }
  monthsLived = leftoverMins / MINS_PER_MONTH;
  daysLived = alarmClock.day - birthDay;
  if (daysLived < 0)
  {
    daysLived += daysInMonth[birthMonth-1];
  }
  hoursLived = alarmClock.hour - birthHour;
  if (hoursLived < 0)
  {
    hoursLived += 24;
  }
  minutesLived = alarmClock.minute - birthMinute;
  if (minutesLived < 0)
  {
    minutesLived += 60;
  }
  secondsLived = alarmClock.second - birthSecond;
  if (secondsLived < 0)
  {
    secondsLived += 60;
  }
}


//***************************************************************************
unsigned long CAppTimeLived::diffDays()
{
  time_t t1 = makeTimeInMinutes(alarmClock.year, alarmClock.month, alarmClock.day, alarmClock.hour, alarmClock.minute);
  return (t1 - birthTime) / 1440;
}


//***************************************************************************
unsigned long CAppTimeLived::diffHours()
{
  time_t t1 = makeTimeInMinutes(alarmClock.year, alarmClock.month, alarmClock.day, alarmClock.hour, alarmClock.minute);
  return (t1 - birthTime) / 60;
}


//***************************************************************************
// returns the number of minutes passed since birth;
//
unsigned long CAppTimeLived::diffMinutes()
{
  time_t t1 = makeTimeInMinutes(alarmClock.year, alarmClock.month, alarmClock.day, alarmClock.hour, alarmClock.minute);
  return (t1 - birthTime);
}



//***************************************************************************
//
void CAppTimeLived::setBirthdate(int8_t hh, int8_t mm, int8_t ss, int16_t year, int8_t month, int8_t day)
{
  birthYear   = year;
  birthMonth  = month;
  birthDay    = day;
  birthHour   = hh;
  birthMinute = mm;
  birthSecond = ss;
}


//***************************************************************************
//
void CAppTimeLived::setBirthdate(tm birthDatetime)
{
  birthYear   = birthDatetime.year;
  birthMonth  = birthDatetime.month;
  birthDay    = birthDatetime.day;
  birthHour   = birthDatetime.hour;
  birthMinute = birthDatetime.minute;
  birthSecond = birthDatetime.second;
}


CAppTimeLived appTimeLived;

#endif
