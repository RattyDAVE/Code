/*
 *********************************************************************************************************
 * AppPacman.h, part of Wise Clock 4 library, by FlorinC (http://timewitharduino.blogspot.com/);
 *
 *********************************************************************************************************
 */


#ifndef _APP_PACMAN_H_
#define _APP_PACMAN_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppBase.h"


class CAppPacman : public CAppBase
{
private:
  int8_t pacPos;

public:
  virtual void init(byte mode=0);
  virtual int16_t run();
};


extern CAppPacman appPacman;


#endif  // _APP_PACMAN_H_
