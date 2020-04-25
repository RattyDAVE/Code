// AppVu.h

#ifndef _APP_VU_H_
#define _APP_VU_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppBase.h"


class CAppVu : public CAppBase
{
  int spectrumValue[7];	// to hold a2d values

public:
  virtual void init(byte mode=0);
  virtual int16_t run();
};


extern CAppVu appVu;


#endif  // _APP_VU_H_


