/*
 *********************************************************************************************************
 * AlarmClock.h
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
//* Jul 05/10 (fc) added utc (read from time.txt file);
//* Feb 05/11 (fc) adding support for setting the date;
//* Jun 20/11 (rp) added isChimeTriggerEnabled, setDateOnly(), incDSTHour(0, decDSTHour();
//* Oct 15/11 (rp) added high / low temperature vars;   
//* May 05/12 (mcm) use int8_t or int16_t instead of int where appropriate
//* Aug 14/13 (mcm) new get12Hour() to replace common code in other modules
//*
//*********************************************************************************************************



#ifndef _ALARM_CLOCK_
#define _ALARM_CLOCK_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif


#define DEFAULT_ALARM_HOUR	7
#define DEFAULT_ALARM_MIN	0

class CAlarmClock
{
public:
  // shows that the user in the process of setting the alarm time;
  boolean isAlarmSetting;

  boolean isTimeSetting;	//*	user is setting TIME;
  boolean isDateSetting;	//*	user is setting the date;
  boolean isDaySetting;		//*	user is setting the day of week;

  // true when the alarm is sounding;
  // alarm is stopped from remote control or, less practical, by pressing any button;
  boolean isAlarmSoundOn;

  // shows if the alarm is enabled or not;
  boolean isAlarmEnabled;

  // it could happen that the alarm is triggered and stopped in the same minute;
  // this boolean prevents the alarm to start again after it was stopped;
  // (mb)
  boolean isAlarmTriggerEnabled;
  boolean isChimeTriggerEnabled;

  // used for stopping the alarm sound after snoozeMins;
  unsigned long timeAlarmSoundStarted;

  boolean	is24hourModeEnabled;		//*	12/24 hour mode
  
  int8_t	hour;
  int8_t	previousHour;
  int8_t	minute;
  int8_t	previousMinute;
  int8_t	second;
  int16_t	year;
  int16_t	previousYear;
  int8_t	month;
  int8_t	previousMonth;
  int8_t	day;
  int8_t	previousDay;
  int8_t	dow;
  int8_t	alarmHour;
  int8_t	alarmMin;
  int16_t	high24Temp;
  int16_t	low24Temp;
  int8_t	high24Hour;
  int8_t	high24Minute;
  int8_t	low24Hour;
  int8_t	low24Minute;

public:
  CAlarmClock();
  void	checkAlarm();
  void	getTimeFromRTC();
  void	initPrevious();
  void	setTime(int8_t hh, int8_t mm, int8_t ss, int16_t year, int8_t month, int8_t day, int8_t dow);
  void	setDateOnly();
  void	incrementTimeSetting (boolean isSettingHours);
  void	incrementAlarmSetting(boolean isSettingHours);
  void	incrementYear();
  void	incrementMonth();
  void	incrementDay();
  void	incrementDOW();
  void	saveTimeAndDate();
  void  incDSTHour();
  void  decDSTHour();
  int8_t get12Hour();
};


extern CAlarmClock alarmClock;


#endif  // _ALARM_CLOCK_

