// AppLines.h

#ifndef _APP_LINES_H_
#define _APP_LINES_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppBase.h"


class CAppLines : public CAppBase
{
private:
  coord_t x1, x2, dx1, dx2;
  int8_t  y1, y2, dy1, dy2;


public:
  virtual void init(byte mode=0);
  virtual int16_t run();

};


extern CAppLines appLines;


#endif  // _APP_LINES_H_
