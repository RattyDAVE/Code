// AppTimeClock.h

#ifndef _APP_TIMECLOCK_H_
#define _APP_TIMECLOCK_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppBase.h"


class CAppTimeClock : public CAppBase
{
public:
  int8_t tcProject;	// current Time Clock Project number;
	
private:
  byte tClockState;
  unsigned int inTime;
  unsigned int totalTime;
  boolean needInit;
 
public:
  virtual void init(byte mode=0);
  virtual int16_t run();
  void	saveTimeClockdetails(int8_t project, byte tClockState, unsigned int inTime, unsigned int totalTime);
  void	saveCurrentTCProject();
  
private:
  void showSubTotal(unsigned int totalTime); 
  void	getTimeClockdetails(int8_t project, byte* tClockState, unsigned int* inTime, unsigned int* totalTime);
  byte 	getCurrentTCProject();

};


extern CAppTimeClock appTimeClock;


#endif  // _APP_TIMECLOCK_H_


