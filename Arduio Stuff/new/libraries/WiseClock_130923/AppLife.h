// AppLife.h

#ifndef _APP_LIFE_H_
#define _APP_LIFE_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppBase.h"


class CAppLife : public CAppBase
{
public:
  virtual void init(byte mode=0);
  virtual int16_t run();

private:
};


extern CAppLife appLife;


#endif  // _APP_LIFE_H_


