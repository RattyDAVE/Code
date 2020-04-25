// AppTix.h

#ifndef _APP_TIX_H_
#define _APP_TIX_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppBase.h"


class CAppTix : public CAppBase
{

private:
  boolean startupAppTix;
  boolean clearTixDisplay;
  byte tixSpeed;
  byte lastSecond;
  
public:
  virtual void init(boolean start);
  virtual int16_t run();

private:
  void setTixColor(byte number, byte first, byte last, byte color);
  
};

extern CAppTix appTix;

#endif  // _APP_TIX_H_


