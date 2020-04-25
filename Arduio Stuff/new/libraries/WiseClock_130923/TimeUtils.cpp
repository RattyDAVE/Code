// TimeUtils.cpp


#include "TimeUtils.h"


char	*dayName[]	=	{"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
char	*monthName[]	=	{"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const byte daysInMonth[12] =	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };



//*************************************************************************************************
// returns the number of minutes passed from Jan 1, 1900 to the given date and time;
// year is given as 4 digit (e.g. 2012);
// This function is used by AppLivesLived (hence the start year is 1900 and not 1970 as in UnixTime).
//
time_t makeTimeInMinutes(int16_t year, int8_t month, int8_t day, int8_t hour, int8_t minute)
{
  int16_t i;

  if (year < 1900 || year > 2050)
    return 0UL;				// so birthTime will be 0;

  year = year - 1900;
  time_t minutes;

  // minutes from 1900 till 1 jan 00:00:00 of the given year;
  minutes= year * MINS_PER_YEAR;
  for (i = 0; i < year; i++)
  {
    if (LEAP_YEAR(i))
    {
      minutes +=  MINS_PER_DAY;   // add extra days for leap years;
    }
  }
  
  // add days for this year, months start from 1;
  for (i = 1; i < month; i++)
  {
    if ( (i == 2) && LEAP_YEAR(year))
    { 
      minutes += MINS_PER_DAY * 29;
    }
    else
    {
      minutes += MINS_PER_DAY * daysInMonth[i-1];  // daysInMonth array starts from 0;
    }
  }
  minutes+= (day-1) * MINS_PER_DAY;
  minutes+= hour * MINS_PER_HOUR;
  minutes+= minute;

  return minutes;
}


//*************************************************************************************************
// calculate and return the number of seconds passed from Jan 1, 1970 up to the given date;
//
time_t makeUnixTime(int16_t year, int8_t month, int8_t day, int8_t hour, int8_t minute, int8_t second)
{
  if (year < 1970)
    return 0UL;

  year = year - 1970;
  
  // seconds from 1970 till 1 jan 00:00:00 of the given year;
  time_t seconds= year*(SECS_PER_DAY * 365);

  int i = 0;
  for (i = 0; i < year; i++)
  {
    if (LEAP_YEAR(i))
    {
      seconds +=  SECS_PER_DAY;   // add extra days for leap years;
    }
  }
  
  // add days for this year, months start from 1;
  for (i = 1; i < month; i++)
  {
    if ( (i == 2) && LEAP_YEAR(year))
    { 
      seconds += SECS_PER_DAY * 29;
    }
    else
    {
      seconds += SECS_PER_DAY * daysInMonth[i-1];  // daysInMonth array starts from 0;
    }
  }

  seconds+= (day-1) * SECS_PER_DAY;
  seconds+= hour * SECS_PER_HOUR;
  seconds+= minute * SECS_PER_MIN;
  seconds+= second;

  return seconds; 
}


//*************************************************************************************************
int16_t julian_day(int16_t y, int8_t m, int8_t d)
{
  int16_t jd = 0;
  int8_t i;

  if ((m > 2) && ((y & 0x3) == 0) && (((y % 100) != 0) || ((y % 400) == 0))) {
    jd++;	// one more for a leap year
  }
  m--;		// array offset starts at zero
  for (i=0; i < m; i++) {
    jd += daysInMonth[i];
  }
  jd += d;

  return(jd);
}

