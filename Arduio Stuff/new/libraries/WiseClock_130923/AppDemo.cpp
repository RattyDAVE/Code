// AppDemo.cpp

#include "UserConf.h"
#ifdef WANT_APP_DEMO

#include "HT1632.h"
#include "AlarmClock.h"
#include "WiseClock.h"
#include "WantedIncludes.h"

#ifndef _APP_BIG_H_
 #define WANT_DEMO_TEXT
#endif


// list of apps to cycle through in 'demo' mode
byte demoApps[] = {
#ifdef WANT_DEMO_TEXT		/* display text of mode */

 #ifdef _APP_LINES_H_
	APP_LINES,		/* moving lines demo, 10 seconds */
 #endif
 #ifdef _APP_QUOTE_H_
	APP_QUOTE,		/* scrolling quotes */
 #endif
 #ifdef _APP_BIG_H_
	APP_BIG,		/* large font clock */
 #endif
 #ifdef _APP_WORDS_H_
	APP_WORDS,		/* time in words */
 #endif
 #ifdef _APP_TIX_H_
	APP_TIX,		/* Tix clock */
 #endif
 #ifdef _APP_PONG_H_
	APP_PONG,		/* Pong game clock */
 #endif
 #ifdef _APP_PACMAN_H_
	APP_PACMAN,		/* Pacman clock */
 #endif
 #ifdef _APP_ANIMATION_H_
	APP_ANIM,		/* Animations */
 #endif
	APP_UTC,		/* UTC analog dials */
 #ifdef _APP_LIFE_H_
	APP_LIFE,		/* life game, 15 seconds */
 #endif

#else	/* big mode's SHOW_ALL between demos */

 #ifdef _APP_QUOTE_H_
  APP_BIG, APP_QUOTE,
 #endif
 #ifdef _APP_PACMAN_H_
  APP_BIG, APP_PACMAN,
 #endif
  APP_BIG, APP_BIG,	// binary (12)
 #ifdef _APP_PONG_H_
  APP_BIG, APP_PONG,
 #endif
  APP_BIG, APP_BIG,	// seconds (0)
 #ifdef _APP_TIX_H_
  APP_BIG, APP_TIX,
 #endif
  APP_BIG, APP_BIG,	// filled hour circle (13)

#endif  // WANT_DEMO_TEXT

	100			/* last in list */
};


#ifdef WANT_DEMO_TEXT

// defined in WiseClock.cpp;
 #ifdef _APP_LINES_H_
extern const char menu_str_lines[];
 #endif
 #ifdef _APP_QUOTE_H_
extern const char menu_str_quote[];
 #endif
 #ifdef _APP_BIG_H_
extern const char menu_str_big[];
 #endif
 #ifdef _APP_TIX_H_
extern const char menu_str_tix[];
 #endif
 #ifdef _APP_WORDS_H_
extern const char menu_str_words[];
 #endif
 #ifdef _APP_PONG_H_
extern const char menu_str_pong[];
 #endif
 #ifdef _APP_PACMAN_H_
extern const char menu_str_pacmn[];
 #endif
 #ifdef _APP_ANIMATION_H_
extern const char menu_str_anim[];
 #endif
 #ifdef _APP_UTC_H_
extern const char menu_str_utc[];
 #endif
 #ifdef _APP_LIFE_H_
extern const char menu_str_life[];
 #endif

const char *demoStr[] = {
 #ifdef _APP_LINES_H_
	menu_str_lines,		// "LINES"
 #endif
 #ifdef _APP_QUOTE_H_
	menu_str_quote,		// "QUOTE"
 #endif
 #ifdef _APP_BIG_H_
	menu_str_big,		// "BIG"
 #endif
 #ifdef _APP_WORDS_H_
	menu_str_words,		// "WORDS"
 #endif
 #ifdef _APP_TIX_H_
	menu_str_tix,		// "TIX"
 #endif
 #ifdef _APP_PONG_H_
	menu_str_pong,		// "PONG"
 #endif
 #ifdef _APP_PACMAN_H_
	menu_str_pacmn,		// "PACMN"
 #endif
 #ifdef _APP_ANIMATION_H_
	menu_str_anim,		// "ANIM"
 #endif
	menu_str_utc,		// "UTC"
 #ifdef _APP_LIFE_H_
	menu_str_life,		// "LIFE"
 #endif
};
#endif	// WANT_DEMO_TEXT



void CAppDemo::init(byte inSetup/*=0*/)
{
#ifdef WANT_DEMO_TEXT
  demoSetDur		=	3;			// minutes for each demo
#else
  demoSetDur		=	8;			// minutes for each demo
#endif
  demoNoFile		=	false;

  demoIndex	=	0;

  if (inSetup)
  {
    demoState	=	1;

    // (fc, Sep 15, 2013) moved from WiseClock::startApp();
    wiseClock.buttonsInUse  = SET_BUTTON + PLUS_BUTTON;
    wiseClock.plusButtonCnt = demoSetDur;
    wiseClock.setButtonCnt  = 0;
  }
  else
  {
    demoState	=	2;

#ifndef WANT_DEMO_TEXT
    demoBigMode	=	1;	// SHOW_ALL
    demoPrevBig	=	12;	// USE_FONT10
#endif

  }

  recordStartAppTime();
}


int16_t CAppDemo::run()
{

	int16_t ms = 100;
	if (demoState == 1)
	{
		char *durmsg = "0 Min";

		if (wiseClock.plusButtonCnt > 9)						// runs from 0 --> 9
			wiseClock.plusButtonCnt = 0;

		durmsg[0] = '0' + wiseClock.plusButtonCnt;
		displayStaticLine(durmsg, 8, ORANGE);

		if (wiseClock.setButtonCnt == 0)						// wait till Set button pressed;
			return(ms);
			
		demoSetDur = wiseClock.plusButtonCnt;
		demoState = 2;
		wiseClock.setButtonCnt = 0;
		wiseClock.plusButtonCnt = 0;
		wiseClock.buttonsInUse = 0;
#ifndef WANT_DEMO_TEXT
		demoBigMode = 1;	// SHOW_ALL
		demoPrevBig = 12;	// USE_FONT10
#endif
		return(ms);
	}

	if (demoState == 2) {		// display the name of the demo
		clearDisplay();
#ifdef WANT_DEMO_TEXT
		strcpy_P(wiseClock.tempBuffer, PSTR("DEMO"));
		displayStaticLine(wiseClock.tempBuffer, 0, GREEN);
		strcpy_P(wiseClock.tempBuffer, demoStr[demoIndex]);
		displayStaticLine(wiseClock.tempBuffer, 8, ORANGE);
		demoState = 3;
		return(2000);
#else
		demoState = 3;
		return(1);
#endif
	}

	if (demoState == 3) {	// initialize the demo
		clearDisplay();
		demoDuration = demoSetDur * 60000 + 10000;		// one minute per button press, plus 10 seconds
		demoStartTime = millis();
		demoState = 4;
		wiseClock.buttonsInUse  = 0;
	}

	if (((long)(millis() - (demoStartTime + demoDuration)) >= 0)
	&& !wiseClock.isLineInProgress())
	{	// switch to next state when time expires
		demoState = 2;
		wiseClock.crtPos = 9999;	// force a fresh start;
		demoIndex++;
		if (demoApps[demoIndex] >= 100) {		// cycle to the begining
			demoIndex = 0;
		}
		return(0);
	}

		
	switch(demoApps[demoIndex])
	{
#ifdef _APP_LINES_H_
#ifdef WANT_DEMO_TEXT
		case APP_LINES:
			if (demoState == 4) {
				appLines.init();
				demoDuration = 10000;	/* override user setting with 10 seconds */
			} else {
				ms = appLines.run();
			}
			break;
#endif	// WANT_DEMO_TEXT
#endif
#ifdef _APP_QUOTE_H_
		case APP_QUOTE:
			if (demoState == 4) {
  #ifdef _APP_ANIMATION_H_
				if (appAnimation.appAnimationState > 1)
					wiseClock.closeLongFile();
  #endif
				appQuote.init();
				wiseClock.startUpAppQuote = false;
				wiseClock.plusButtonCnt = 0;
  #ifndef WANT_DEMO_TEXT
				demoBigMode = 1;	// SHOW_ALL
  #endif
				wiseClock.crtPos = 9999;	// force a fresh start;
			} else {
				ms = appQuote.run();
			}
			break;
#endif
#ifdef _APP_BIG_H_
		case APP_BIG:
			if (demoState == 4) {
  #ifdef WANT_DEMO_TEXT
				appBig.init(14);			// CIRCULATE_BIG_MODES
    #ifdef WANT_ANIMATIONS
				demoInAnim = false;
    #endif

  #else	// cycle through SHOW_ALL (time/temperature) between demos
				appBig.init(demoBigMode);
				if (demoBigMode == 1) {			// SHOW_ALL
					if (demoSetDur == 0)
						demoDuration = 12000;
					else
						demoDuration = 24000;
					demoBigMode = demoPrevBig;
    #ifdef WANT_ANIMATIONS
					demoInAnim = true;
    #endif
				} else {
					if (demoPrevBig == 0)		// SHOW_SECONDS
						demoPrevBig = 12;	// USE_FONT10
					else if (demoPrevBig == 12)
						demoPrevBig = 13;	// USE_FONT11
					else
						demoPrevBig = 0;	// SHOW_SECONDS
					demoBigMode = 1;		// SHOW_ALL
    #ifdef WANT_ANIMATIONS
					demoInAnim = false;
    #endif
				}
  #endif // WANT_DEMO_TEXT
			} else {
				ms = appBig.run();
  #ifdef WANT_ANIMATIONS
				if (appAnimation.isAnimEnabled) {
					if ((alarmClock.minute % 15) == 0) {
						if (demoInAnim == false) {
							demoInAnim = true;
							appAnimation.showQuarter(alarmClock.minute);
						}
					} else {
						if (demoBigMode == 1) {		// SHOW_ALL
							demoInAnim = false;
						}
					}
				}
  #endif
			}
			break;
#endif
#ifdef _APP_TIX_H_
		case APP_TIX:
			if (demoState == 4) {
				appTix.init(false);
  #ifndef WANT_DEMO_TEXT
				demoBigMode = 1;	// SHOW_ALL
  #endif
			} else {
				ms = appTix.run();
			}
			break;
#endif
#ifdef _APP_WORDS_H_
#ifdef WANT_DEMO_TEXT
		case APP_WORDS:
			if (demoState == 4) {
				appWords.init(false);
				demoNoFile = !appWords.openWordsFile(appWords.getWordsFileDigit());
			} else {
				if (demoNoFile) {
					demoDuration = 0;
				} else {
					ms = appWords.run();
				}
			}
			break;
#endif	// WANT_DEMO_TEXT
#endif
#ifdef _APP_PONG_H_
		case APP_PONG:
			if (demoState == 4) {
				appPong.init();
  #ifndef WANT_DEMO_TEXT
				demoBigMode = 1;	// SHOW_ALL
  #endif
			} else {
				ms = appPong.run();
			}
			break;
#endif
#ifdef _APP_PACMAN_H_
		case APP_PACMAN:
			if (demoState == 4) {
				appPacman.init();
  #ifndef WANT_DEMO_TEXT
				demoBigMode = 1;	// SHOW_ALL
  #endif
			} else {
				ms = appPacman.run();
			}
			break;
#endif
#ifdef _APP_ANIMATION_H_
#ifdef WANT_DEMO_TEXT
		case APP_ANIM:
			if (demoState == 4) {
				appAnimation.init();
				appAnimation.appAnimationState = 3;
				demoNoFile = !wiseClock.openLongFile("anim1.wc3");
			} else {
				if (demoNoFile) {
					demoDuration = 0;
				} else {
					ms = appAnimation.run();
				}
			}
			break;
#endif	// WANT_DEMO_TEXT
#endif
#ifdef _APP_UTC_H_
#ifdef WANT_DEMO_TEXT
		case APP_UTC:
			if (demoState == 4) {
				appUtc.init(3);
			} else {
				ms = appUtc.run();
			}
			break;
#endif	// WANT_DEMO_TEXT
#endif
#ifdef _APP_LIFE_H_
#ifdef WANT_DEMO_TEXT
		case APP_LIFE:
			if (demoState == 4) {
				appLife.init();
				demoDuration = 15000;	// override user setting with 15 seconds
			} else {
				ms = appLife.run();
			}
			break;
#endif	// WANT_DEMO_TEXT
#endif
		default:
			demoIndex = 0;
			break;

	}
	if (demoState == 4)
		demoState = 5;
	return(ms);
}


CAppDemo appDemo;
#endif
