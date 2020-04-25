// AppUtc.h

#ifndef _APP_UTC_H_
#define _APP_UTC_H_

#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppBase.h"


class CAppUtc : public CAppBase
{
public:

private:
  byte utcState;

public:
  virtual void init(byte st=0);
  virtual int16_t run();
  void	 saveUtcDiff();
  int8_t getUtcDiff();

private:
  void fetchUtcTime();
  void showAnaClock(coord_t o, int8_t u, boolean utc, boolean showSecs);
};


extern CAppUtc appUtc;

#endif
