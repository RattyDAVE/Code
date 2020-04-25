// AppAnimation.h

#ifndef _APP_ANIMATION_H_
#define _APP_ANIMATION_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppBase.h"
#include "WiseClock.h"


struct animHeader
{
	byte fileId1;
	byte fileId2;
	byte delay1;
	byte delay2;
	byte delay3;
	byte delay4;
	byte brightness1;
	byte brightness2;
	byte indicator;
	byte repeatCount1;
	byte repeatCount2;
	byte repeatCount3;
	byte repeatCount4;
};	

// indicator contains the following bits;
#define LASTSCREEN  0x08
#define STARTREPEAT 0x04
#define   ENDREPEAT 0x02


class CAppAnimation : public CAppBase
{

private:
	unsigned int delayTime, repeatCnt, randomDelayTime;
	unsigned long repeatPos;
	byte bIndicator1, bIndicator2, bValue;
	byte allFileCount, endIndicator;
	boolean quarterlyRunning, allFiles, randomDelayFlag, inRepeatCycle;
	
public:	
	byte appAnimationState;
	boolean isAnimEnabled;		// show (or not) quarterly animation;
	boolean isAnimOpen;		// animation file is open


public:
	virtual void init(byte mode=0);
	virtual int16_t run();
	boolean getAnim();
	void  saveAnim();
	byte  getAnimFileDigit();
	void  saveAnimFileDigit(byte animFileDigit);

	void showQuarter(byte minute);
};


extern CAppAnimation appAnimation;


#endif  // _APP_ANIMATION_H_


