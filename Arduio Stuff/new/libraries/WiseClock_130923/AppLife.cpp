// AppLife.cpp

//*********************************************************************************************************
//*	Edit History, started June, 2011
//*	please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//*	Jun  20,	2011	(rp) added random color to "case 3";
//*     Apr  07,        2012    (mcm) corrected neighbor counts, change color progression (green = birth, orange = alive, red = death)
//*     Apr  18,        2012    (mcm) wrap around edge of display
//*     May  28,        2012    (mcm) restructure to eliminate delay() calls
//*	Sep  01,	2013	(fc) return to the app running before Life was launched after a certain period;
//
//*********************************************************************************************************


#include "UserConf.h"
#ifdef WANT_APP_LIFE

#include "AppLife.h"
#include "HT1632.h"
#include "WiseClock.h"


#define kDemoTimeoutMilliSeconds 60000


#ifdef RESTRICT_SCROLL_DISPLAY
 #undef X_MAX
 #define X_MAX 32
#endif


void CAppLife::init(byte setup/*=0*/)
{
  if (!setup)	// (fc, Sep 15, 2013) moved from WiseClock::startApp();
  {
	// start a new game of life (from a newly generated patern);
	char randomText[3]	=	{0};
	randomText[0]		=	random(32, 127);
	randomText[1]		=	random(32, 127);
	randomText[2]		=	random(32, 127);
	displayStaticLine(randomText, 0, GREEN);

	recordStartAppTime();
  }
}


int16_t CAppLife::run()
{
	byte newval, oldval, isalive, change;
	int8_t y, ny, i, j, neighbors;
	coord_t x, nx;

	snapshot_shadowram();

	change = 0;
	for (x=0; x < X_MAX; x++) {
		for (y=0; y < Y_MAX; y++) {
			neighbors = 0;
			for (i=-1; i<=1; i++) {
				nx = x + i;
				if (nx < 0) nx = X_MAX-1;
				else if (nx > X_MAX-1) nx = 0;
				for (j=-1; j<=1; j++) {
					ny = y + j;
					if (ny < 0) ny = Y_MAX-1;
					else if (ny > Y_MAX-1) ny = 0;

					if (i==0 && j==0)
						oldval = get_snapshotram(x, y);
					else
						neighbors += get_snapshotram(nx, ny) & 1;
				}
			}
			isalive = oldval & 1;

			switch (neighbors)
			{
				case 0:
				case 1:
					newval = isalive ? RED : BLACK;		// death by loneliness
					break;

				case 2:
					newval = isalive ? ORANGE : BLACK;	// remains the same
					break;

				case 3:
					newval = isalive ? ORANGE: GREEN;	// Birth if empty/dead
					break;

				default:
					newval = isalive ? RED : BLACK;		// death by overcrowding
					break;
			}

			if (oldval != newval) {
				ht1632_plot(x, y, newval);
				change = 1; // display has changed
			}
		}
	}

	delay(500);

	if (!change)
	{
		// static display, so start over
		init();
	}

	if (hasAppTimePassed(kDemoTimeoutMilliSeconds))
	{
		clearDisplay();

		// return to the app that ran before Life;
		wiseClock.restoreCrtApp();
		if (wiseClock.crtApp == APP_QUOTE)
		{
			wiseClock.crtPos = 9999;
			wiseClock.startUpAppQuote = false;
		}
		wiseClock.pCrtApp->init();
	}

}

CAppLife appLife;

#endif
