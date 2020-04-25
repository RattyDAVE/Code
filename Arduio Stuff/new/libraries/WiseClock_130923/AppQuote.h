// AppQuote.h

#ifndef _APP_QUOTE_H_
#define _APP_QUOTE_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppBase.h"


class CAppQuote : public CAppBase
{
private:
#ifdef WANT_QUOTE_RANDOM
  boolean doRand;	// true == quotes in random order
  uint8_t rnd;		// quote lines to skip
#endif
  uint8_t quoteState;
public:
  virtual void init(byte mode=0);
  virtual int16_t run();
  byte  getQuoteFileDigit();
  void  saveQuoteFileDigit(byte quoteFileDigit);
};


extern CAppQuote appQuote;


#endif  // _APP_QUOTE_H_


