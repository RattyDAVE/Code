// AppStopWatch.h

#ifndef _APP_STOPWATCH_H_
#define _APP_STOPWATCH_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppBase.h"



class CAppStopWatch : public CAppBase
{
public:
   
private:
 boolean isSetup;
 boolean isStopped;

 byte hour, minute, second, prevSecond;
 byte tensec;
 long startTime;

 byte chronoMode;
 void AddOneSecond();

  // (fc, Sep 21, 2013) save/read chrono mode;
  byte getChronoMode();
  void saveChronoMode();


public:
  virtual void init(byte mode=0);
  virtual int16_t run();
};


extern CAppStopWatch appStopWatch;


#endif  // _APP_STOPWATCH_H_


