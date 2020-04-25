// AppLines.cpp

//*********************************************************************************************************
//*	Edit History, started June, 2011
//*	please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//*	Jun  20,	2011	(rp) added random color to bres_line();
//*	May  28,	2012	(mcm) restructure to eliminate delay() calls
//*	Aug  07,	2013	(mcm) use coord_t instead of char
//*	Aug  13,	2013	(mcm) simplify bres_line()
//*
//*********************************************************************************************************

#include "UserConf.h"
#ifdef WANT_APP_LINES

#include "HT1632.h"
#include "AppLines.h"

#ifdef RESTRICT_SCROLL_DISPLAY
 #undef X_MAX
 #define X_MAX 32
#endif


void CAppLines::init(byte mode/*=0*/)
{
	clearDisplay();
	x1	=	random(0,X_MAX);
	x2	=	random(0,X_MAX);
	y1	=	random(0,Y_MAX);
	y2	=	random(0,Y_MAX);
	dx1	=	random(1,4);
	dx2	=	random(1,4);
	dy1	=	random(1,4);
	dy2	=	random(1,4);
}


//*********************************************************************************************************
/*
 * demo_bouncyline
 * The endpoints of a line move independently and bounce off the edges of the display.
 */
int16_t CAppLines::run()
{
	ht1632_line(x1,y1, x2,y2, BLACK);

	x1 += dx1;
	if (x1 > X_MAX) {
		x1	=	X_MAX;
		dx1	=	-random(1,4);
	} else if (x1 < 0) {
		x1	=	0;
		dx1	=	random(1,4);
	}

	x2 += dx2;
	if (x2 > X_MAX) {
		x2	=	X_MAX;
		dx2	=	-random(1,4);
	} else if (x2 < 0) {
		x2	=	0;
		dx2	=	random(1,4);
	}

	y1 += dy1;
	if (y1 > Y_MAX) {
		y1	=	Y_MAX;
		dy1	=	-random(1,3);
	} else if (y1 < 0) {
		y1	=	0;
		dy1	=	random(1,3);
	}

	y2 += dy2;
	if (y2 > Y_MAX) {
		y2	=	Y_MAX;
		dy2	=	-random(1,3);
	} else if (y2 < 0) {
		y2	=	0;
		dy2	=	random(1,3);
	}
	ht1632_line(x1,y1, x2,y2, random(GREEN, ORANGE + 1));
	return(30);
}

CAppLines appLines;
#endif
