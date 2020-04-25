// AppTimeLived.h

#ifndef _APP_TIMELIVED_H_
#define _APP_TIMELIVED_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppBase.h"
#include "TimeUtils.h"


class CAppTimeLived : public CAppBase
{
public:
  virtual void init(byte mode=0);
  virtual int16_t run();

  void setBirthdate(int8_t hh, int8_t mm, int8_t ss, int16_t year, int8_t month, int8_t day);
  void setBirthdate(tm birthDatetime);

  
private:
  uint16_t birthYear;
  uint8_t  birthMonth;
  uint8_t  birthDay;
  uint8_t  birthHour;
  uint8_t  birthMinute;
  uint8_t  birthSecond;

  time_t birthTime;  // calculated as number of minutes since Jan 1, 1900;

  int16_t yearsLived, monthsLived, daysLived;
  int16_t hoursLived, minutesLived, secondsLived;

  byte modeSelected;  // time-passed, days-only, hours-only, minutes-only;

  // calculate difference between the current time and time of birth;
  void diffTime();
  unsigned long diffDays();
  unsigned long diffHours();
  unsigned long diffMinutes();
};


extern CAppTimeLived appTimeLived;


#endif  // _APP_TIMELIVED_H_


