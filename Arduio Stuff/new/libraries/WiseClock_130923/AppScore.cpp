// AppScore.cpp, 
 
//*********************************************************************************************************
//*	Edit History, started Oct, 2011
//*	please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//*	Oct  15,	2011	(rp) first entry;
//*	May  28,	2012	(mcm) restructure to eliminate delay() calls
//*	Jun  25,	2012	(mcm) new args to displayTime()
//*	Sep  21,	2013	(fc) added ability to save and restore score from eeprom;
//*
//*********************************************************************************************************



#include "UserConf.h"
#ifdef WANT_APP_SCORE

#include "AppScore.h"
#include "HT1632.h"
#include "WiseClock.h"
#include "AlarmClock.h"

#ifdef MULTI_DISPLAY_SCROLL
 #define X_OFFSET 16
 #define MAX_SCORE 255
#else
 #define X_OFFSET 0
 #define MAX_SCORE 99
#endif


#define BIG_SCORE_FONT		1
#define SKINNY_SCORE_FONT	4


void CAppScore::init(byte bFreshScore /*=0*/)
{
  //---------------------------------------------------------------------------------------
  // (fc, Sep 21, 2013) if Score is the default app (config by the user in PRWON),
  //                    resume the counting from the values before the power down;
  //---------------------------------------------------------------------------------------

	wiseClock.buttonsInUse = SET_BUTTON + PLUS_BUTTON;
	clearDisplay();

	if (bFreshScore)
	{
		score1 = 0;
		score2 = 0;
	}
	else
	{
		getScores();
	}

}


int16_t CAppScore::run()
{
    if (wiseClock.plusButtonCnt > 0)
    {
        wiseClock.plusButtonCnt = 0;

        score1++;
        saveScore1();

        if (score1 > MAX_SCORE)
	    score1 = 0;
    }

    if (wiseClock.setButtonCnt > 0)
    {
        wiseClock.setButtonCnt = 0;

        score2++;
        saveScore2();

        if (score2 > MAX_SCORE)
	    score2 = 0;
    }
		
    if (wiseClock.largeTextFont == true)
    {
#ifdef MULTI_DISPLAY_SCROLL
        byte hundreds = score1 / 100;
        // 10 is the index for blank;
	ht1632_putBigDigit( 0, 2, (hundreds!=0) ? hundreds : 10, BIG_SCORE_FONT, GREEN,  6);
        byte tens = (score1 % 100) / 10;
	ht1632_putBigDigit( 8, 2, (hundreds!=0 || tens!=0) ? tens : 10, BIG_SCORE_FONT, GREEN,  6);
	ht1632_putBigDigit(16, 2, score1%10, BIG_SCORE_FONT, GREEN,  6);

        hundreds = score2 / 100;
	ht1632_putBigDigit(39, 2, (hundreds!=0) ? hundreds : 10, BIG_SCORE_FONT, ORANGE, 6);
        tens = (score2 % 100) / 10;
	ht1632_putBigDigit(47, 2, (hundreds!=0 || tens!=0) ? tens : 10, BIG_SCORE_FONT, ORANGE, 6);
	ht1632_putBigDigit(55, 2, score2 %10, BIG_SCORE_FONT, ORANGE, 6);
#else
	ht1632_putBigDigit( 0, 2, score1/10, SKINNY_SCORE_FONT, GREEN,  6);
	ht1632_putBigDigit( 7, 2, score1%10, SKINNY_SCORE_FONT, GREEN,  6);
	ht1632_putBigDigit(18, 2, score2/10, SKINNY_SCORE_FONT, ORANGE, 6);
	ht1632_putBigDigit(25, 2, score2%10, SKINNY_SCORE_FONT, ORANGE, 6);
#endif
        ht1632_putchar(13 + X_OFFSET, 3, ':', RED);
    }
    else
    {
        // build the string containing the formatted score;
        wiseClock.tempBuffer[0]   =   (score1 < 10) ? ' ' : ('0' + score1/10);
        wiseClock.tempBuffer[1]   =   '0' + score1 % 10;
        wiseClock.tempBuffer[2]   =   ':';
        wiseClock.tempBuffer[3]   =   (score2 < 10) ? ('0' + score2) : ('0' + score2/10);
        wiseClock.tempBuffer[4]   =   (score2 < 10) ? ' ' : ('0' + score2 % 10);

        ht1632_putchar( 2 + X_OFFSET, 0, wiseClock.tempBuffer[0], GREEN);
        ht1632_putchar( 8 + X_OFFSET, 0, wiseClock.tempBuffer[1], GREEN);
        ht1632_putchar(13 + X_OFFSET, 0, wiseClock.tempBuffer[2], RED);
        ht1632_putchar(19 + X_OFFSET, 0, wiseClock.tempBuffer[3], ORANGE);
        ht1632_putchar(25 + X_OFFSET, 0, wiseClock.tempBuffer[4], ORANGE);

        wiseClock.displayTime(8, false);
    }

    return(100);
}


//*********************************************************************************************************
// Read and write the target endtime for countdown (unsigned long, that is 4 bytes, as little endian);
//
void CAppScore::getScores()
{
	score1 = wiseClock.readUserSetting(score1Loc);
	score2 = wiseClock.readUserSetting(score2Loc);
}

void CAppScore::saveScore1()
{
	wiseClock.saveUserSetting(score1Loc, score1);
}

void CAppScore::saveScore2()
{
	wiseClock.saveUserSetting(score2Loc, score2);
}


CAppScore appScore;

#endif

