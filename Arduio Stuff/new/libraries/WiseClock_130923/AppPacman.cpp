/*
 *********************************************************************************************************
 * AppPacman.cpp, part of Wise Clock 4 library, by FlorinC (http://timewitharduino.blogspot.com/);
 *
 *********************************************************************************************************
 */

#include "UserConf.h"
#ifdef WANT_APP_PACMAN

#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppPacman.h"
#include "HT1632.h"
#include "AlarmClock.h"
#include "WiseClock.h"



//*****************************************************************************************************
void CAppPacman::init(byte mode /*=0*/)
{
	pacPos = -16;
	clearDisplay();
}


//*****************************************************************************************************
// this function is called as part of the main loop;
//
int16_t CAppPacman::run()
{
#ifdef MULTI_DISPLAY_SCROLL
  if (alarmClock.second > 52)
#else
  if (alarmClock.second > 55)
#endif
  {
    // display alternatively the pacman bitmaps;
    if (pacPos > -14)
    {
      ht1632_putBitmap(pacPos, 1, abs(pacPos)%7);
      pacPos--;
    } else {
	clearDisplay();
    }
  }
  else
  {
    wiseClock.displayTime(5, true);
#ifdef MULTI_DISPLAY_SCROLL
    pacPos = X_MAX;
#else
    pacPos = 32;
#endif
  }
  return(80);

}


CAppPacman appPacman;
#endif
