// AppBig.h

#ifndef _APP_BIG_H_
#define _APP_BIG_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppBase.h"


// There are 10 BIG Fonts, with 11 characters each, one character is 12 bytes long
#define  NUMBER_OF_FONTS 10
#define  CHARS_IN_FONT   11  //  0 - 9 + Space
#define  BYTES_PER_CHARS 12


class CAppBig : public CAppBase
{
public:
  byte bigStartMode;

private:
  byte bigModeSelected;
  byte currentBigFont;
  byte overlayMode;
  byte blinkingDot;
  boolean inCirculateMode;
  boolean showDate;
  boolean notYetCleared;
  boolean overlayInProgress;
  int overlayX;
  int overlayY;
  byte dispAllDone;
  byte lastHour, lastMinute, lastSecond;

public:
  virtual void 	init(byte mode = 0);
  virtual int16_t run();

private:
  void  displayBigSeconds();
  void 	displayDotsOnly();
  void 	displayBinary();
  void	displayInverse();
  void	displayBigFont();
  void	displayHrCircle();
  byte 	displayAll();
  byte 	doOverlayScreen(); 

};


extern CAppBig appBig;


#endif  // _APP_BIG_H_


