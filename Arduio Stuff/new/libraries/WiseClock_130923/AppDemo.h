// AppDemo.h

#ifndef _APP_DEMO_H_
#define _APP_DEMO_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppBase.h"


class CAppDemo : public CAppBase
{
public:
  virtual void init(byte mode=0);
  virtual int16_t run();

  void setReturnApp(CAppBase* pReturnApp, byte returnApp);

  int8_t	demoState;		// state of current demo
  int8_t	demoIndex;		// index into demo table
  uint8_t	demoSetDur;		// user-set run time of a demo, in minutes
  byte		demoBigMode;
  byte		demoPrevBig;
  unsigned long demoDuration;		// run time of a demo, in ms
  unsigned long demoStartTime;		// run time timer
  boolean  demoNoFile;			// flag file doesn't exist

  boolean  demoInAnim;			// animation in progress

  int16_t demoCycle();

private:
};


extern CAppDemo appDemo;


#endif  // _APP_DEMO_H_


