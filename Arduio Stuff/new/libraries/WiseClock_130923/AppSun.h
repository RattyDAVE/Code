// AppSun.h

#ifndef _APP_SUN_H_
#define _APP_SUN_H_

#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppBase.h"


class CAppSun : public CAppBase
{
public:
  byte blinkFlg;		// are we blinking?
  unsigned long blinkTim;	// when to change blink state

private:
  double sunv[3];
  byte last_min;		// minute of last update
  byte blinkCol;		// saved color of blinking dot
  coord_t blinkX;		// X coordinate of blinking dot
  int8_t blinkY;		// Y coordinate of blinking dot

public:
  virtual void init(byte mode=0);
  virtual int16_t run();

  void blinkLatLon(int16_t lat, int16_t lon);
  void blinkDot(boolean active);

private:
  void latlon2xyz(double lat, double lon, double *xyz);
  void sun_pointing(int day, int minute, double *earth_to_sun);
  double dot(double *v0, double *v1, int n);
  double sun_el(double lat, double lon, double *sunv);
  uint8_t getColor(double el);
  char setPixel(coord_t column, int8_t row, double *sunv);
};

extern CAppSun appSun;

#endif
