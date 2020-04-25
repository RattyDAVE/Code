// AppScore.h

#ifndef _APP_SCORE_H_
#define _APP_SCORE_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppBase.h"


class CAppScore : public CAppBase
{
private:
  byte score1, score2;

  // (fc, Sep 21, 2013) support functions to store and retrieve the score between power downs;
  void getScores();
  void saveScore1();
  void saveScore2();


public:
  virtual void init(byte mode=0);
  virtual int16_t run();

};


extern CAppScore appScore;


#endif  // _APP_SCORE_H_
