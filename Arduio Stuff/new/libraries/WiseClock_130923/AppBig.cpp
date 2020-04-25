// AppBig.cpp

//*********************************************************************************************************
//*	Edit History, started July, 2011
//*	please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//*	Oct  15,	2011	(rp) Code moved from wiseClock3.cpp to here, added displayAll();
//*
//*	May  01,	2012	(mcm) while scrolling, let set button change scroll speed instead of font
//*	May  02,	2012	(mcm) use #define for scroll prefix
//*	May  02,	2012	(mcm) temperature scale now x4, not x10
//*	May  03,	2012	(rp) Corrections for temperature;
//*	May  04,	2012	(mcm) alternate colon/dot blink for alarm indicator
//*	May  05,	2012	(mcm) use int8_t or int16_t instead of int where appropriate
//*	May  06,	2012	(mcm) add binary clock display
//*	May  06,	2012	(mcm) allow "Demo" app to cycle fonts
//*	May  06,	2012	(mcm) cycle fonts at 12 seconds instead of 10
//*	May  28,	2012	(mcm) restructure to eliminate delay() calls
//*	Jun  25,	2012	(mcm) changes to better handle multiple displays
//*	Aug  23,	2012	(mcm) new mode with continuous fetchExtraInfo() scroll
//*	Aug  14,	2013	(mcm) use get12Hour() instead of calculating 12-hour mode
//*	Sep  06,	2013	(mcm) new mode with hour digits & a slowly filled circle
//*********************************************************************************************************

#include "UserConf.h"
#ifdef WANT_APP_BIG

#include "HT1632.h"
#include "WiseClock.h"
#include "TimeUtils.h"
#include "AppTmp.h"
#include "AlarmClock.h"
#include "Sound.h"
#include "AppBig.h"


#ifdef MULTI_DISPLAY_SCROLL
 #define X_OFFSET 8
 #define X_OFFSET_2 16
#else
 #define X_OFFSET 0
 #define X_OFFSET_2 0
#endif

enum						// Big Font modes; 
{
	SHOW_SECONDS = 0,			// show hour, minute and seconds;
	SHOW_ALL,				// show date, time, alarm time and temperature;
	SHOW_EXTRA,				// only show "extra" info
	USE_FONT1,
	USE_FONT2,
	USE_FONT3,
	USE_FONT4,				// show inverse characters;
	USE_FONT5,
	USE_FONT6,
	USE_FONT7,				// show numbers as columns;
	USE_FONT8,				// show numbers as dots;
	USE_FONT9,				// show bcd numbers;
	USE_FONT10,				// show binary numbers;
	USE_FONT11,				// hours as numbers, minutes as cyclic fill of a circle
	CIRCULATE_BIG_MODES			// Circulate through all big modes;
};


void CAppBig::init(byte mode)
{
	clearDisplay();

	lastHour = 24;

	bigModeSelected = mode;					// User selected big mode;
	if (bigModeSelected > CIRCULATE_BIG_MODES)
	    bigModeSelected = SHOW_SECONDS;
	inCirculateMode = (bigModeSelected == CIRCULATE_BIG_MODES);	// automatic changing big modes;

	notYetCleared = true;					// flag for clearing the big display if hour go from 1 -> 2 digits v.v.;
	wiseClock.setButtonCnt = 0;				// Set buttom used to step to the next big mode;
	
	recordStartAppTime();		// Start new cycle

	dispAllDone = 0;
	overlayX = 0 + X_OFFSET_2;
	overlayY = 0;
	overlayInProgress = false;
}


//*********************************************************************************************************
// (fc, Aug 1/2010) display time (H12:MM) with big font, on the entire screen;
// (rp, Oct 2010) modified to show different fonts, blinking dot etc;
//
int16_t CAppBig::run()
{
    byte bigColor;
    int16_t dly;

    if (wiseClock.isLineInProgress())
    {
	wiseClock.buttonsInUse = 0;			// Set scrolling speed while scrolling; 
	if (wiseClock.largeTextFont == false)
	    wiseClock.displayTime(8, false);
	return(100);
    }	
    if (wiseClock.buttonsInUse == 0)
    {
	if (bigModeSelected != SHOW_EXTRA)
		clearDisplay();

	recordStartAppTime();  	// Start new cycle after end of message
	dispAllDone = 0;
	overlayInProgress = false;
    }	

    wiseClock.buttonsInUse = SET_BUTTON;	// Set button is used in Big mode; 
    blinkingDot = BLACK;
    bigColor = wiseClock.getColor();

    if (wiseClock.setButtonCnt != 0)
    {
	wiseClock.setButtonCnt = 0;
	if ((inCirculateMode) || (++bigModeSelected > CIRCULATE_BIG_MODES))
	    bigModeSelected = SHOW_SECONDS;
	inCirculateMode = (bigModeSelected == CIRCULATE_BIG_MODES);	// automatic changing big modes;
	bigStartMode = bigModeSelected;
	clearDisplay();
	notYetCleared = false;
	dispAllDone = 0;
	overlayInProgress = false;

	recordStartAppTime(); 	// Start new cycle
    }
	
    if (bigModeSelected > CIRCULATE_BIG_MODES)
        bigModeSelected = SHOW_SECONDS;
    if ((bigModeSelected <= USE_FONT11) && (bigModeSelected >= USE_FONT1))
	currentBigFont = bigModeSelected - USE_FONT1;
    else
	currentBigFont = 0;

    dly = 100;
    switch (bigModeSelected)
    {
	case SHOW_SECONDS:
	    displayBigSeconds();
	    break;
			
	case SHOW_ALL:				// show date, time, alarm time and temp in C and F;
	    dly = displayAll();
	    break;

	case SHOW_EXTRA:
	    wiseClock.displayTime(8, false);
	    break;

	case USE_FONT4:				// show inverse mode;
	    displayInverse();
	    displayBigFont();
	    break;

	case USE_FONT7:
	case USE_FONT8:
	case USE_FONT9:
	    displayDotsOnly();
	    break;
			
	case USE_FONT10:
	    displayBinary();
	    break;
			
	case USE_FONT11:
	    displayHrCircle();
	    break;

	default:
	    displayBigFont();
	    break;
    }
	
    // start new cycle after 12 seconds;
    if ((bigModeSelected == SHOW_EXTRA && hasAppTimePassed(1000)) || hasAppTimePassed(12000))
    {
	recordStartAppTime();

	if (inCirculateMode)
	{
	    if (++bigModeSelected > CIRCULATE_BIG_MODES)	// Change Font every 12 seconds
		bigModeSelected = SHOW_SECONDS;
	    clearDisplay();
	}
		
	if ((wiseClock.fetchExtraInfo() == true) && (bigModeSelected != SHOW_EXTRA))
	    clearDisplay();
	dispAllDone = 0;
	overlayInProgress = false;
    }
    return(dly);
}


//*********************************************************************************************************
// Display big hours and small seconds;
//
void CAppBig::displayBigSeconds()
{
    byte hour   = alarmClock.get12Hour();
    byte minute = alarmClock.minute;
    byte second = alarmClock.second;
    boolean isAlarmEnabled = alarmClock.isAlarmEnabled;
    byte bigColor = wiseClock.getColor();
    coord_t off;

    if (hour > 9)
    {
	if ((hour == 10) && (notYetCleared == true))
	{
	    clearDisplay();					// to prevent flickering only clear the display one time;
	    notYetCleared = false;
	}	
	if ((hour == 12) || (hour == 23))
	    notYetCleared = true;
		
#ifdef MULTI_DISPLAY_SCROLL
	ht1632_putBigDigit(12, 2, hour / 10, 5, bigColor, 6);
#else
	ht1632_putBigDigit( 0, 2, hour / 10, 5, bigColor, 6);
	ht1632_putBigDigit( 6, 2, hour % 10, 5, bigColor, 6);
	ht1632_putBigDigit(14, 2, minute/10, 5, bigColor, 6);
	ht1632_putBigDigit(20, 2, minute%10, 5, bigColor, 6);
	ht1632_putchar(26, 0, '0' + second/10, bigColor | SMALL_CHAR);
	ht1632_putchar(26, 7, '0' + second%10, bigColor | SMALL_CHAR);
#endif
	off = 13;
    }	
    else
    {
	if (((hour == 0) || (hour == 1)) && (notYetCleared == true))
	{
	    clearDisplay();					// to prevent flickering only clear the display one time;
	    notYetCleared = false;
	}	
	if (hour == 9)
	    notYetCleared = true;
		
#ifndef MULTI_DISPLAY_SCROLL
	ht1632_putBigDigit( 1, 2, hour,      5, bigColor, 6);
	ht1632_putBigDigit( 9, 2, minute/10, 5, bigColor, 6);
	ht1632_putBigDigit(15, 2, minute%10, 5, bigColor, 6);
	ht1632_putchar(21, 4, '0' + second/10, bigColor | SMALL_CHAR);
	ht1632_putchar(26, 4, '0' + second%10, bigColor | SMALL_CHAR);
#endif
	off = 8;
    }	
#ifdef MULTI_DISPLAY_SCROLL
    ht1632_putBigDigit(18, 2, hour % 10, 5, bigColor, 6);
    ht1632_putBigDigit(26, 2, minute/10, 5, bigColor, 6);
    ht1632_putBigDigit(32, 2, minute%10, 5, bigColor, 6);
    ht1632_putBigDigit(40, 2, second/10, 5, bigColor, 6);
    ht1632_putBigDigit(46, 2, second%10, 5, bigColor, 6);
#endif
    if ((second & 1) == 0)
    {
#ifdef MULTI_DISPLAY_SCROLL
	ht1632_plot(25,  5, bigColor);
	ht1632_plot(25, 10, bigColor);
	ht1632_plot(39,  5, bigColor);
	ht1632_plot(39, 10, bigColor);
#else
	ht1632_plot(   off,  5, bigColor);
	ht1632_plot(   off, 10, bigColor);
#endif
    }
    else
    {
#ifdef MULTI_DISPLAY_SCROLL
	ht1632_plot(25,  5, BLACK);
	ht1632_plot(25, 10, isAlarmEnabled ? bigColor : BLACK);
	ht1632_plot(39,  5, BLACK);
	ht1632_plot(39, 10, isAlarmEnabled ? bigColor : BLACK);
#else
	ht1632_plot(   off,  5, BLACK);
	ht1632_plot(   off, 10, isAlarmEnabled ? bigColor : BLACK);
#endif
    }
}	


//*********************************************************************************************************
// Display extra lines for inverse font 4;
//
void CAppBig::displayInverse()
{
	coord_t i;
	byte bigColor = wiseClock.getColor();

	blinkingDot = bigColor;
	
#ifdef MULTI_DISPLAY_SCROLL
	for (i=1; i<47; i++)
#else
	for (i=1; i<31; i++)
#endif
	{
		ht1632_plot(i+X_OFFSET,  1, bigColor);     	// draw extra horizontal lines
		ht1632_plot(i+X_OFFSET, 14, bigColor);    
	}

	for (i=2; i<14; i++)
	{
		ht1632_plot( 7+X_OFFSET, i, bigColor);    	// draw extra vertical lines
		ht1632_plot( 8+X_OFFSET, i, bigColor);    
		ht1632_plot(14+X_OFFSET, i, bigColor);    
		if ((i != 5) && (i != 10))		// skip colon area
		{
			ht1632_plot(15+X_OFFSET, i, bigColor);    
			ht1632_plot(16+X_OFFSET, i, bigColor);    
#ifdef MULTI_DISPLAY_SCROLL
			ht1632_plot(31+X_OFFSET, i, bigColor);    
			ht1632_plot(32+X_OFFSET, i, bigColor);    
#endif
		}
		ht1632_plot(17+X_OFFSET, i, bigColor);     
		ht1632_plot(23+X_OFFSET, i, bigColor);     
		ht1632_plot(24+X_OFFSET, i, bigColor);     
		ht1632_plot(30+X_OFFSET, i, bigColor);     
#ifdef MULTI_DISPLAY_SCROLL
		ht1632_plot(33+X_OFFSET, i, bigColor);     
		ht1632_plot(39+X_OFFSET, i, bigColor);     
		ht1632_plot(40+X_OFFSET, i, bigColor);     
		ht1632_plot(46+X_OFFSET, i, bigColor);     
#endif
	}
}


//*********************************************************************************************************
// Display time in dots only;
//
void CAppBig::displayDotsOnly()
{
    byte hour   = alarmClock.get12Hour();
    byte minute = alarmClock.minute;
    byte second = alarmClock.second;
    byte bigColor = wiseClock.getColor();
	
    ht1632_putBigDigit( 0+X_OFFSET_2, 2, hour / 10, currentBigFont, bigColor, 5);
    ht1632_putBigDigit( 5+X_OFFSET_2, 2, hour % 10, currentBigFont, bigColor, 5);
    ht1632_putBigDigit(11+X_OFFSET_2, 2, minute/10, currentBigFont, bigColor, 5);
    ht1632_putBigDigit(16+X_OFFSET_2, 2, minute%10, currentBigFont, bigColor, 5);
    ht1632_putBigDigit(22+X_OFFSET_2, 2, second/10, currentBigFont, bigColor, 5);
    ht1632_putBigDigit(27+X_OFFSET_2, 2, second%10, currentBigFont, bigColor, 5);
}


//*********************************************************************************************************
// Display time in binary;
//
void CAppBig::displayBinary()
{
    byte hour   = alarmClock.get12Hour();
    byte minute = alarmClock.minute;
    byte second = alarmClock.second;
    byte val;
    byte bigColor = wiseClock.getColor();
	
    // combine each bit of hours, minutes, seconds into a 0 - 7 range
    val = ((second & 32) >> 5) | ((minute & 32) >> 4) | ((hour & 32) >> 3);
    ht1632_putBigDigit( 1+X_OFFSET_2, 2, val, currentBigFont, bigColor, 5);

    val = ((second & 16) >> 4) | ((minute & 16) >> 3) | ((hour & 16) >> 2);
    ht1632_putBigDigit( 6+X_OFFSET_2, 2, val, currentBigFont, bigColor, 5);

    val = ((second & 8) >> 3) | ((minute & 8) >> 2) | ((hour & 8) >> 1);
    ht1632_putBigDigit(11+X_OFFSET_2, 2, val, currentBigFont, bigColor, 5);

    val = ((second & 4) >> 2) | ((minute & 4) >> 1) | (hour & 4);
    ht1632_putBigDigit(16+X_OFFSET_2, 2, val, currentBigFont, bigColor, 5);

    val = ((second & 2) >> 1) | (minute & 2) | ((hour & 2) << 1);
    ht1632_putBigDigit(21+X_OFFSET_2, 2, val, currentBigFont, bigColor, 5);

    val = (second & 1) | ((minute & 1) << 1) | ((hour & 1) << 2);
    ht1632_putBigDigit(26+X_OFFSET_2, 2, val, currentBigFont, bigColor, 5);
}


//*********************************************************************************************************
// Display hour as digits, filling circle for minutes
//
const byte partTable[30] PROGMEM = {
    0x06,
    0x16, 0x13, 0x26, 0x35, 0x46, 0x45, 0x33,
    0x44, 0x54, 0x64, 0x53, 0x31, 0x62, 0x61,
    0x60,
    0x61, 0x31, 0x62, 0x53, 0x64, 0x54, 0x33,
    0x44, 0x45, 0x46, 0x35, 0x13, 0x26, 0x16
};

const byte circ[6] PROGMEM = {
    0x07, 0x17, 0x27, 0x36, 0x46, 0x55
};

void CAppBig::displayHrCircle()
{
    byte hour   = alarmClock.get12Hour();
    byte bigColor = wiseClock.getColor();
    int8_t i, ind, quad, ye;
    coord_t xe;
	
    if ((lastHour != hour)
    ||  (get_shadowram(25 + X_OFFSET_2, 0) != bigColor)
    ) {
	clearDisplay();
	lastSecond = 30;
	lastMinute = 60;

	// display the hours
	ht1632_putBigDigit(1 + X_OFFSET_2, 2, hour/10, 5, bigColor, 6);
	ht1632_putBigDigit(8 + X_OFFSET_2, 2, hour%10, 5, bigColor, 6);
	lastHour = hour;
	
	// draw the circle
	for (i=5; i>=0; i--) {
	    ind = pgm_read_byte_near(&circ[i]);
	    xe = ind & 0xf;
	    ye = ind >> 4;
	    ht1632_plot(24+X_OFFSET_2+xe,  7+ye, bigColor);
	    ht1632_plot(24+X_OFFSET_2-xe,  7+ye, bigColor);
	    ht1632_plot(24+X_OFFSET_2+xe,  7-ye, bigColor);
	    ht1632_plot(24+X_OFFSET_2-xe,  7-ye, bigColor);
	    ht1632_plot(24+X_OFFSET_2+ye,  7+xe, bigColor);
	    ht1632_plot(24+X_OFFSET_2-ye,  7+xe, bigColor);
	    ht1632_plot(24+X_OFFSET_2+ye,  7-xe, bigColor);
	    ht1632_plot(24+X_OFFSET_2-ye,  7-xe, bigColor);
	}
    }

    // fill the block
    if ((lastMinute != alarmClock.minute) && (alarmClock.minute != 0)) {
	lastMinute = alarmClock.minute;
	for (i=0; i<=lastMinute; i++) {
	    ind = i;
	    quad = 0;
	    if (ind >= 30) {
		ind -= 30;
		quad = 2;
	    }
	    if (ind >= 15) quad++;
	    ind = pgm_read_byte_near(&partTable[ind]);
	    xe = ind >> 4;
	    ye = ind & 0xf;
	    if (quad == 0 || quad == 3) {
		ye = -ye;
	    }
	    if (quad & 2) {
		xe = -xe;
	    }
	    ht1632_line(24+X_OFFSET_2, 7, 24+X_OFFSET_2+xe, 7+ye, bigColor);
	}
    }
    if (alarmClock.second != lastSecond) {
	ht1632_plot(X_OFFSET_2 + 1 + (lastSecond>>1),  15, BLACK);
	lastSecond = alarmClock.second;
	ht1632_plot(X_OFFSET_2 + 1 + (lastSecond>>1),  15, RED);
    }
}


//*********************************************************************************************************
// Display time using one of the big fonts;
//
void CAppBig::displayBigFont()
{
    byte hour   = alarmClock.get12Hour();
    byte minute = alarmClock.minute;
    boolean isAlarmEnabled = alarmClock.isAlarmEnabled;
    byte bigColor = wiseClock.getColor();
	
    if (hour > 9)
	ht1632_putBigDigit(1+X_OFFSET, 2, hour/10, currentBigFont, bigColor, 6);
    else
	ht1632_putBigDigit(1+X_OFFSET, 2, 10, currentBigFont, bigColor, 6);  // 10 = space

    ht1632_putBigDigit( 8+X_OFFSET, 2, hour % 10, currentBigFont, bigColor, 6);
    ht1632_putBigDigit(17+X_OFFSET, 2, minute/10, currentBigFont, bigColor, 6);
    ht1632_putBigDigit(24+X_OFFSET, 2, minute%10, currentBigFont, bigColor, 6);
#ifdef MULTI_DISPLAY_SCROLL
    ht1632_putBigDigit(33+X_OFFSET, 2, alarmClock.second/10, currentBigFont, bigColor, 6);
    ht1632_putBigDigit(40+X_OFFSET, 2, alarmClock.second%10, currentBigFont, bigColor, 6);
#endif

    byte altColor = (blinkingDot == BLACK) ? bigColor : BLACK;
    if ((alarmClock.second & 1) == 0)
    {
	ht1632_plot(15+X_OFFSET,  5, altColor);
	ht1632_plot(16+X_OFFSET,  5, altColor);
	ht1632_plot(15+X_OFFSET, 10, altColor);
	ht1632_plot(16+X_OFFSET, 10, altColor);
#ifdef MULTI_DISPLAY_SCROLL
	ht1632_plot(31+X_OFFSET,  5, altColor);
	ht1632_plot(32+X_OFFSET,  5, altColor);
	ht1632_plot(31+X_OFFSET, 10, altColor);
	ht1632_plot(32+X_OFFSET, 10, altColor);
#endif
    }
    else
    {
	ht1632_plot(15+X_OFFSET,  5, blinkingDot);
	ht1632_plot(16+X_OFFSET,  5, blinkingDot);
#ifdef MULTI_DISPLAY_SCROLL
	ht1632_plot(31+X_OFFSET,  5, blinkingDot);
	ht1632_plot(32+X_OFFSET,  5, blinkingDot);
#endif
	if (isAlarmEnabled)
	{
	    ht1632_plot(15+X_OFFSET, 10, altColor);
	    ht1632_plot(16+X_OFFSET, 10, altColor);
#ifdef MULTI_DISPLAY_SCROLL
	    ht1632_plot(31+X_OFFSET, 10, altColor);
	    ht1632_plot(32+X_OFFSET, 10, altColor);
#endif
	}
	else
	{
	    ht1632_plot(15+X_OFFSET, 10, blinkingDot);
	    ht1632_plot(16+X_OFFSET, 10, blinkingDot);
#ifdef MULTI_DISPLAY_SCROLL
	    ht1632_plot(31+X_OFFSET, 10, blinkingDot);
	    ht1632_plot(32+X_OFFSET, 10, blinkingDot);
#endif
	}
    }
}


//*********************************************************************************************************
// Display date, day, time, alarm time, temp in C and F;
//
byte CAppBig::displayAll()
{
	byte color;
	boolean isAlarmEnabled = alarmClock.isAlarmEnabled;

	if (overlayInProgress)
		return(doOverlayScreen());

	clearSnapshot(0);
	if (!hasAppTimePassed(4000))              		// if 4 seconds have not yet passed; 
	{
		if (dispAllDone <= 0) {
			clearDisplay();
			strcpy(wiseClock.tempBuffer, dayName[alarmClock.dow]);	// show date;
			ht1632_putchar( 7 + X_OFFSET_2, 0, wiseClock.tempBuffer[0], GREEN | SMALL_CHAR | PUTINSNAPSHOTRAM);
			ht1632_putchar(13 + X_OFFSET_2, 0, wiseClock.tempBuffer[1], GREEN | SMALL_CHAR | PUTINSNAPSHOTRAM);
			ht1632_putchar(19 + X_OFFSET_2, 0, wiseClock.tempBuffer[2], GREEN | SMALL_CHAR | PUTINSNAPSHOTRAM);

			strcpy(wiseClock.tempBuffer, monthName[alarmClock.month]);
			ht1632_putchar( 1 + X_OFFSET_2, 8, wiseClock.tempBuffer[0], GREEN | SMALL_CHAR | PUTINSNAPSHOTRAM);
			ht1632_putchar( 7 + X_OFFSET_2, 8, wiseClock.tempBuffer[1], GREEN | SMALL_CHAR | PUTINSNAPSHOTRAM);
			ht1632_putchar(13 + X_OFFSET_2, 8, wiseClock.tempBuffer[2], GREEN | SMALL_CHAR | PUTINSNAPSHOTRAM);

			ht1632_putchar(21 + X_OFFSET_2, 8, '0' + alarmClock.day / 10, GREEN | SMALL_CHAR | PUTINSNAPSHOTRAM);
			ht1632_putchar(26 + X_OFFSET_2, 8, '0' + alarmClock.day % 10, GREEN | SMALL_CHAR | PUTINSNAPSHOTRAM);
			overlayX = 0 + X_OFFSET_2;
			overlayY = 0;
			overlayInProgress = true;
			dispAllDone = 1;
			if (++overlayMode > 2)
				overlayMode = 0;
		}
	}
	else if (!hasAppTimePassed(8000))           // if 4 -> 8 second period 
	{        
		byte hour   = alarmClock.alarmHour;
		byte minute = alarmClock.alarmMin;

		if (dispAllDone <= 1) {
			clearDisplay();

			ht1632_putchar( 1 + X_OFFSET_2, 0, 'A', GREEN | SMALL_CHAR | PUTINSNAPSHOTRAM);			// Show time + alarm time;
			ht1632_putchar( 8 + X_OFFSET_2, 0, '0' + hour / 10, ORANGE | SMALL_CHAR | PUTINSNAPSHOTRAM);
			ht1632_putchar(13 + X_OFFSET_2, 0, '0' + hour % 10, ORANGE | SMALL_CHAR | PUTINSNAPSHOTRAM);

			color = wiseClock.getColor();
			ht1632_plot(19 + X_OFFSET_2, 5, ORANGE | PUTINSNAPSHOTRAM);
			ht1632_plot(19 + X_OFFSET_2, 13, color | PUTINSNAPSHOTRAM);

			if (!isAlarmEnabled)
			{
				ht1632_plot(19 + X_OFFSET_2, 3, ORANGE | PUTINSNAPSHOTRAM);
				ht1632_plot(19 + X_OFFSET_2, 11, color | PUTINSNAPSHOTRAM);
			}
			ht1632_putchar(20 + X_OFFSET_2, 0, '0' + minute / 10, ORANGE | SMALL_CHAR | PUTINSNAPSHOTRAM);
			ht1632_putchar(25 + X_OFFSET_2, 0, '0' + minute % 10, ORANGE | SMALL_CHAR | PUTINSNAPSHOTRAM);

			hour   = alarmClock.hour;
			minute = alarmClock.minute;

			ht1632_putchar( 1 + X_OFFSET_2, 8, 'T', GREEN | SMALL_CHAR | PUTINSNAPSHOTRAM);
			ht1632_putchar( 8 + X_OFFSET_2, 8, '0' + hour / 10, color | SMALL_CHAR | PUTINSNAPSHOTRAM);
			ht1632_putchar(13 + X_OFFSET_2, 8, '0' + hour % 10, color | SMALL_CHAR | PUTINSNAPSHOTRAM);

			ht1632_putchar(20 + X_OFFSET_2, 8, '0' + minute / 10, color | SMALL_CHAR | PUTINSNAPSHOTRAM);
			ht1632_putchar(25 + X_OFFSET_2, 8, '0' + minute % 10, color | SMALL_CHAR | PUTINSNAPSHOTRAM);
			overlayX = 0 + X_OFFSET_2;
			overlayY = 0;
			overlayInProgress = true;
			dispAllDone = 2;
		}
	}
	else if (!hasAppTimePassed(12000))           		// if 8 -> 12 second period 
	{        
		int16_t temp, iTemp, dTemp;

		if (dispAllDone <= 2) {
			clearDisplay();

			temp = appTemperature.getTemperature();			// Get Celsius Temperature
			wiseClock.splitTemp(temp, &iTemp, &dTemp, true);
				
			ht1632_putchar( 3 + X_OFFSET_2, 0, '0' + iTemp / 10, ORANGE | SMALL_CHAR | PUTINSNAPSHOTRAM);
			ht1632_putchar( 8 + X_OFFSET_2, 0, '0' + iTemp % 10, ORANGE | SMALL_CHAR | PUTINSNAPSHOTRAM);
			ht1632_plot(14 + X_OFFSET_2, 6, ORANGE | SMALL_CHAR | PUTINSNAPSHOTRAM);
			ht1632_putchar(15 + X_OFFSET_2, 0, '0' + dTemp, ORANGE | SMALL_CHAR | PUTINSNAPSHOTRAM); 
			ht1632_putchar(21 + X_OFFSET_2, 0, 0x7f, ORANGE | SMALL_CHAR | PUTINSNAPSHOTRAM);
			ht1632_putchar(27 + X_OFFSET_2, 0, 'C', GREEN | SMALL_CHAR | PUTINSNAPSHOTRAM);
				
			wiseClock.splitTemp(temp, &iTemp, &dTemp, false);
			ht1632_putchar(-2 + X_OFFSET_2, 8, (iTemp < 100) ? ' ' : '0' + iTemp / 100, ORANGE | SMALL_CHAR | PUTINSNAPSHOTRAM);
			iTemp %= 100;
			ht1632_putchar( 3 + X_OFFSET_2, 8, '0' + iTemp / 10, ORANGE | SMALL_CHAR | PUTINSNAPSHOTRAM);
			ht1632_putchar( 8 + X_OFFSET_2, 8, '0' + iTemp % 10, ORANGE | SMALL_CHAR | PUTINSNAPSHOTRAM);
			ht1632_plot(14 + X_OFFSET_2, 14, ORANGE | PUTINSNAPSHOTRAM);
			ht1632_putchar(15 + X_OFFSET_2, 8, '0' + dTemp, ORANGE | SMALL_CHAR | PUTINSNAPSHOTRAM); 
			ht1632_putchar(21 + X_OFFSET_2, 8, 0x7f, ORANGE | SMALL_CHAR | PUTINSNAPSHOTRAM);
			ht1632_putchar(27 + X_OFFSET_2, 8, 'F', GREEN | SMALL_CHAR | PUTINSNAPSHOTRAM);
			overlayX = X_OFFSET_2;
			overlayY = 0;
			overlayInProgress = true;
			dispAllDone = 3;
		}
	}
	return(0);
}	


byte CAppBig::doOverlayScreen()
{
	byte dly = 0;
	if (overlayMode == 0)
	{
		if (overlayX < 32 + X_OFFSET_2)
		{
			overlayWithSnapshotVertical(overlayX);
			overlayX++;
			dly = 32;
		}
		else
		{
			overlayInProgress = false;
		}
	}
	else
	{
		if (overlayY < 16)
		{
			overlayWithSnapshotHorizontal(overlayY);
			overlayY++;
			if (overlayMode != 1)
				dly = 64;
		}
		else
		{
			overlayInProgress = false;
		}
	}
	return(dly);
}


CAppBig appBig;
#endif
