// AppCntDown.h

#ifndef _APP_CNTDOWN_H_
#define _APP_CNTDOWN_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppBase.h"
#include "TimeUtils.h"


class CAppCntDown : public CAppBase
{
private:
  boolean cntDownStartUp;
  boolean freezeTime;
  int day, hour, minute, second;

  long cntDownTime;	// number of seconds to count;
  long startTime;	// countdown start time, as number of seconds from the beginning of the day (0:0:0);

  int blinkingDigit;
  int8_t loopCnt;

  // (fc, Sep 14, 2013) support functions to resume counting down after the power is on again;
  time_t getEndTime();
  void   saveEndTime(time_t endTimeInSeconds);


public:
  virtual void init(byte bFreshCounting=0);
  virtual int16_t run();
};


extern CAppCntDown appCntDown;


#endif  // _APP_CNTDOWN_H_


