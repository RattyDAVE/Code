// TimeUtils.h


#ifndef _TIMEUTILS_H_
#define _TIMEUTILS_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif



typedef struct
{
  int8_t  second; 
  int8_t  minute; 
  int8_t  hour; 
  int8_t  dow;   // day of week, Sunday is day 1
  int8_t  day;
  int8_t  month; 
  int16_t year;
} tm;


typedef unsigned long time_t;

// calculate and return the number of minutes passed from Jan 1, 1900 up to the given date;
time_t makeTimeInMinutes(int16_t year, int8_t month, int8_t day, int8_t hour, int8_t minute);

// calculate and return the number of seconds passed from Jan 1, 1970 up to the given date;
time_t makeUnixTime(int16_t year, int8_t month, int8_t day, int8_t hour, int8_t minute, int8_t second);


int16_t julian_day(int16_t y, int8_t m, int8_t d);


// leap year calculator expects year argument as years offset from 1900
#define LEAP_YEAR(Y)     ( ((1900+Y)>0) && !((1900+Y)%4) && ( ((1900+Y)%100) || !((1900+Y)%400) ) )

#define MINS_PER_HOUR  (60UL)
#define MINS_PER_DAY   (1440UL)
#define MINS_PER_YEAR  (525600UL) 
#define MINS_PER_MONTH (43800UL)   // 365 / 12 * 24 * 60

#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24UL)


extern char *dayName[];		// 3-character day-of-week name (zero based)
extern char *monthName[];	// 3-character month name (1-based)
extern const byte daysInMonth[];	// number of days in each month (zero based)


#endif	// _TIMEUTILS_H_


