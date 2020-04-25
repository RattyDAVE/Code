/*
 *********************************************************************************************************
 * UserConf.h
 *
 * Sep/13 by FlorinC (http://timewitharduino.blogspot.com/)
 *   Copyrighted and distributed under the terms of the Berkeley license
 *   (copy freely, but include this notice of original authors.)
 *
 *********************************************************************************************************
 */

//*********************************************************************************************************
//*     Edit History, started September, 2013
//*     please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//* Sep 10/13 (mcm) initial version
//*********************************************************************************************************


//
// Common user-configurable options for WiseClock
//
#ifndef _WISE_CLOCK_CONF_H_
#define _WISE_CLOCK_CONF_H_

// Hardware configuration
#define _WISE_CLOCK_VER		4	// version of wise clock hardware
#define NUM_DISPLAYS		2	// how many 16x32 display boards

// define the following if you want scrolling limited to one display
// only meaningful if more than one display is connected
// useful when the second display is the day/night "Sun" map, and the
// first display is the normal WiseClock.
//#define RESTRICT_SCROLL_DISPLAY


//
// Select the applications you want included
// Comment out the #define line of the
// applications you don't want.
//
#define WANT_APP_QUOTE			// scroll of quotations
#define WANT_APP_UTC			// analog or text clock of UTC
#define WANT_APP_BIG			// various "big" fonts
#define WANT_APP_LIFE			// Conway's game of life
#define WANT_APP_DEMO			// demo of most of the apps
#define WANT_APP_PONG			// pong display, score is time
#define WANT_APP_PACMAN			// animated pacman on the minute
#define WANT_APP_LIVED			// elapsed time since date on SD card
#define WANT_APP_SCORE			// two-person score
#define WANT_APP_STOPW			// stop watch
#define WANT_APP_CNT_DOWN		// count down timer
#define WANT_APP_WORDS			// display time in words
#define WANT_APP_TCLOK			// time spent on different projects
#define WANT_APP_TIX			// TIX clock display
#define WANT_APP_LINES			// random lines bounce on display
//#define WANT_APP_SUN			// day/night graph, needs map overlay
#define WANT_APP_ANIM			// animations read from SD card


//
// Software options
//

// used to enable/disable Serial.print, time-consuming operation;
// comment out this line if you want more RAM available;
//#define _DEBUG_

// used to enable/disable processing of messages from the second serial port
// takes the place of the personal message on the SD card
//#define SERIAL_MSG

// used to select "software serial" instead of the 2nd UART for SERIAL_MSG
// A JY-MCU Bluetooth module connected to D20 and D21 needs this
#define USE_SOFTWARE_SERIAL

// used to select WiFly XBee module connected to the 2nd UART
#define WANT_WIFLY

// used to enable/disable the option to display the time in various cities
// the cities are defined on the SD card
#define WANT_TIME_AT_CITY

// option to display animations on quarter hour
#define WANT_ANIMATIONS

// option to log temperature to the SD card on the hour
#define WANT_EVENT_LOG

// option to change demo display separator.
// default is to separate demos with big mode's "SHOW_ALL" (time/temperature)
// With this option set, demos are separated with text display of mode
//#define WANT_DEMO_TEXT

// option to display quotes in more random order
#define WANT_QUOTE_RANDOM

#endif
