// AppTmp.h

#ifndef _APP_TMP_H_
#define _APP_TMP_H_



#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppBase.h"


// start with Fahrenheit for our US friends
#define DEFAULT_USE_CELSIUS	false



class CAppTmp : public CAppBase
{
public:
  virtual void init(byte mode=0);
  virtual int16_t run();

  boolean useCelsius;	// use Celsius or Fahrenheit?

  int	getTemperature();

  void saveUseCelsius();
  boolean getUseCelsius();

  int getTempOffset();
  void saveTempOffset(byte tempCorrection);
};


extern CAppTmp appTemperature;


#endif  // _APP_TMP_H_
