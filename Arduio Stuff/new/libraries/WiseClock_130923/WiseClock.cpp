/*
 *********************************************************************************************************
 * WiseClock.cpp
 *
 * Apr/10 by FlorinC (http://timewitharduino.blogspot.com/)
 *   Copyrighted and distributed under the terms of the Berkeley license
 *   (copy freely, but include this notice of original authors.)
 *
 *********************************************************************************************************
 */
//*********************************************************************************************************
//*	Edit History, started April, 2010
//*	please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//*	Apr  1,	2010	<MLS> modified to use the non-C++ version of tone that is in verison 018 of Arduino);
//*	Apr  2,	2010	<MLS> Added timeout to LIFE and DEMO, currently it is 1 minute (60 seconds)
//*	Apr  2,	2010	<MLS> Added blink to the hours/minutes when setting alarm
//*	Apr  2,	2010	<MLS> Added TIME menu to set the TIME, basically the same as setting the alarm
//*	Apr  3,	2010	<MLS> Added 12/24 hour display mode
//*
//* Apr 15/10 (fc) created class, by restructuring Wise4Sure.pde;
//* Apr 16/10 (fc) new features: 1. display personalized message read from SD card; 2. introduced comments;
//* Jul 05/10 (fc) new feature: read (from time.txt file), store and display UTC time;
//* Aug 01/10 (fc) new feature: display only time on the whole screen, with big font;
//* Oct 10/10 (rp) new features for Big Font display: 8 more fonts added (use + key to select next font) 
//*                If "CIRCULATE_BIG_MODES" is selected then font will change every 5 seconds. Date and message can also        
//*                be displayed in Big font mode. Changing alarm, date or message setting will now return  
//*				   to Big font mode
//* Jan 29/11 (fc) adapted for 3216 bi-color display;
//* Jan 30/11 (fc) added new features:
//*	- show temperature;
//*	- different colors for message, date and temperature;
//*	- save/retrieve user settings to/from internal eeprom;
//*	- set date and day-of-week;
//* Jun 10/11 (fc) - added mode Pacman;
//*   - fixed dimming (through button Set);
//*	Jun 20/11 (rp) added reminders, chime, DST, score, Fahrenheit, seconds to big fonts, 
//*   colored time when close to alarm time, updated SD file handling (did not read last sector)
//*   menu will no longer show DATE+ if date already set etc.
//* Oct 15/11 (rp) added Stopwatch, Countdown, Words, Tix, Stats, Time clock, large Font, Celc/Fahr;
//* Mar 11/12 (fc) added "Time lived" feature, to show how many years/days/hours etc have passed since birth;
//* Mar 25/12 (fc) replaced SDuFAT with SdFat, which supports FAT32 as well as FAT16;
//* Apr 07/12 (mcm) quotation lines now have unlimited length
//* Apr 12/12 (mcm) unify WiseClock3/WiseClock4 code
//* Apr 12/12 (mcm) added DST back, changed UTC code to use DST flag
//* Apr 15/12 (mcm) removed floating point in favor of scaled integers
//* Apr 16/12 (mcm) quotation lines can start with '^R' for red, '^O' for orange
//* Apr 19/12 (mcm) have 'DEMO' app cycle through other apps
//* Apr 24/12 (mcm) move menu strings to PROGMEM
//* Apr 27/12 (mcm) move quote, Utc apps to their own file
//* Apr 27/12 (mcm) remove '^R', '^O' color prefix code
//* Apr 27/12 (mcm) new feature: '<R>', '<O>', '<G>' anywhere in scrolling line change color for the rest of the line
//* May 01/12 (mcm) option to display time at various cities (#define WANT_TIME_AT_CITY)
//* May 01/12 (mcm) option to display message from serial1 port (Bluetooth?) (#define SERIAL_MSG)
//* May 02/12 (mcm) use #define for scroll prefix
//* May 05/12 (mcm) alternate colon/dot blink for alarm indicator
//* May 05/12 (mcm) use int8_t or int16_t instead of int where appropriate
//* May 06/12 (mcm) added flag argument to appBig.init() for demo mode
//* May 28/12 (mcm) restructure to eliminate delay() calls
//* May 29/12 (mcm) add #ifdef's to make it easy to pick and choose what apps you want in the WiseClock
//* Jun 17/12 (mcm) Convert from ifstream to SdFile
//* Jun 17/12 (mcm) incorporate Ruud's animation/event log/temperature changes:
//* May 02/12 (rp) added animations and logging;
//* May 02/12 (rp) Correction for temperature is no longer hard coded as 3 but can be changed in the Cels/Fahr menu;
//* May 02/12 (rp) Power down and up time stamps are now written to log file;
//* May 02/12 (rp) All temperature conversions are now done in 1 spot;
//* Jun 19/12 (mcm) add NEWSD app to close & re-read SD file system
//* Jun 25/12 (mcm) changes to better support multiple displays
//* Aug 23/12 (mcm) fix lock-up upon SD card read error
//* Aug 26/12 (mcm) new clearLine() routine to clear a textual line of the display
//* Aug 26/12 (mcm) add offset for multiple displays during time, date settings
//* Sep 06/12 (mcm) move more strings to PROGMEM
//* Sep 06/12 (mcm) allow serial messages to set alarm, enable/disable alarm, silence alarm
//* Aug 14/13 (mcm) use get12Hour() instead of calculating 12-hour mode
//* Aug 15/13 (mcm) add PWRON app to set app to run at power-on
//* Sep 01/13 (fc)  created CAppBase class so that all app classes should derive from it;
//* Sep 01/13 (fc)  moved code for Pacman and Demo from WiseClock.cpp into their own app classes;
//* Sep 14/13 (fc)  if AppCntDown is default, resume counting down after power is restored;
//
//  TODO:
//  - add code for tilt sensor;
//  - hourglass app for vertical display;
//  - countdown, stopwatch, score incremented remotely (BT, XBee etc);
//*
//*********************************************************************************************************

#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif
#include <avr/pgmspace.h>
#include "EEPROM.h"		// save/retrieve user settings;
#include "DS3231.h"		// save date/time when power went down in RTC RAM;
#include "HT1632.h"
#include "TimeUtils.h"
#include "AppTmp.h"		// includes temperature-related items;
#include "WiseClock.h"
#include "AlarmClock.h"
#include "Sound.h"
#include "Buttons.h"
#include "WantedIncludes.h"


//---------------------------------------------------------------------
// (fc, Mar 25/12) replaced SDuFAT with SdFat to support FAT32 as well;
#include <SdFat.h>
SdFat	 sd;		// file system object
SdFile sdLong;		// input stream for long-duration open files;
SdFile sdShort;		// stream for short-duration open files;

#if defined(USE_SOFTWARE_SERIAL) && defined(SERIAL_MSG)
  #include <SoftwareSerial.h>
  SoftwareSerial swSerial1 = SoftwareSerial(21, 20);
#endif


// display menu item for this duration of time, then exit menu automatically;
#define TIME_KEEP_MENU_ACTIVE	8000


const char SdFailMsg[] PROGMEM	= "Failed to read from the SD card...  ";

//  menu item indexes;
//	these MUST match the table below
enum 
{
	MENU_SETUP		=	0,		// Menu 1 = Apps;
#ifdef _APP_QUOTE_H_
	MENU_QUOTE,
#endif
#ifdef _APP_BIG_H_
	MENU_BIG,
#endif
#ifdef _APP_TIX_H_
	MENU_TIX,
#endif
#ifdef _APP_WORDS_H_
	MENU_WORDS,
#endif
#ifdef _APP_CNTDOWN_H_
	MENU_CNT_DOWN,
#endif
#ifdef _APP_STOPWATCH_H_
	MENU_STOPW,
#endif
#ifdef _APP_SCORE_H_
	MENU_SCORE,
#endif
#ifdef _APP_PONG_H_
	MENU_PONG,
#endif
#ifdef _APP_PACMAN_H_
	MENU_PAC,
#endif
#ifdef _APP_TIMELIVED_H_
	MENU_LIVED,
#endif
#ifdef _APP_ANIMATION_H_
	MENU_ANIM,
#endif
#ifdef _APP_TIMECLOCK_H_
	MENU_TCLOK,
#endif
	MENU_UTC,

#ifdef _APP_SUN_H_
 #if NUM_DISPLAYS < 2
	MENU_SUN,
 #endif
#endif

#ifdef _APP_LIFE_H_
	MENU_LIFE,
#endif
#ifdef _APP_DEMO_H_
	MENU_DEMO,
#endif
	MENU_NEWSD,
	MENU_STATS,
	
	MENU_APPS,					// Menu 2 = Setup;
	MENU_SET_ALARM,
	MENU_ALARM_ON,
	MENU_ALARM_OFF,
	MENU_DATE_ON,
	MENU_DATE_OFF,
	MENU_REMIND_ON,
	MENU_REMIND_OFF,
	MENU_TEMP_ON,
	MENU_TEMP_OFF,
	MENU_MSG_ON,
	MENU_MSG_OFF,
#ifdef WANT_TIME_AT_CITY
	MENU_CITY_ON,
	MENU_CITY_OFF,
#endif
	MENU_CHIME_ON,
	MENU_CHIME_OFF,
#ifdef WANT_ANIMATIONS
	MENU_ANIM_ON,
	MENU_ANIM_OFF,
#endif
	MENU_LFONT_ON,
	MENU_LFONT_OFF,
#ifdef WANT_EVENT_LOG
        MENU_LOG_ON,
        MENU_LOG_OFF,
        MENU_LOG_CLEAR,
#endif
	MENU_CELSIUS_ON,
	MENU_CELSIUS_OFF,
	MENU_24_HOUR_ON,
	MENU_24_HOUR_OFF,
	MENU_DST_ON,
	MENU_DST_OFF,
	MENU_SET_TIME,
	MENU_SET_DATE,
	MENU_SET_DAY,
	MENU_SET_POWER_ON,
};

// maximum number of menu items; increment it for each new menu item;
#define MAX_ITEM_MENU1		MENU_STATS
#define MAX_ITEM_MENU2		MENU_SET_POWER_ON


// menu items;
const char menu_str_setup[]	PROGMEM = "SETUP";	//*	Go to Menu 2 with all Set Up options;
#ifdef _APP_QUOTE_H_
extern const char menu_str_quote[]	PROGMEM = "QUOTE";
#endif
#ifdef _APP_BIG_H_
extern const char menu_str_big[]	PROGMEM = "BIG";	// (fc) added Aug 1/2010 to display time only, with big font;
#endif
#ifdef _APP_TIX_H_
extern const char menu_str_tix[]	PROGMEM = "TIX";
#endif
#ifdef _APP_WORDS_H_
extern const char menu_str_words[]	PROGMEM = "WORDS";
#endif
#ifdef _APP_CNTDOWN_H_
extern const char menu_str_cntdn[]	PROGMEM = "CNTDN";
#endif
#ifdef _APP_STOPWATCH_H_
extern const char menu_str_stopw[]	PROGMEM = "STOPW";
#endif
#ifdef _APP_SCORE_H_
extern const char menu_str_score[]	PROGMEM = "SCORE";
#endif
#ifdef _APP_PONG_H_
extern const char menu_str_pong[]	PROGMEM = "PONG";
#endif
#ifdef _APP_PACMAN_H_
extern const char menu_str_pacmn[]	PROGMEM = "PACMN";
#endif
#ifdef _APP_TIMELIVED_H_
extern const char menu_str_lived[]	PROGMEM = "LIVED";	// (fc, Mar 14/2012) time lived (years, months, days, hours, minutes, seconds);
#endif
#ifdef _APP_ANIMATION_H_
extern const char menu_str_anim[]	PROGMEM = "ANIM";
#endif
#ifdef _APP_TIMECLOCK_H_
extern const char menu_str_tclok[]	PROGMEM = "TCLOK";
#endif
extern const char menu_str_utc[]	PROGMEM = "UTC";

#ifdef _APP_SUN_H_
 #if NUM_DISPLAYS < 2
extern const char menu_str_sun[]	PROGMEM = "SUN";
 #endif
#endif

#ifdef _APP_LIFE_H_
extern const char menu_str_life[]	PROGMEM = "LIFE";
#endif
#ifdef _APP_DEMO_H_
extern const char menu_str_demo[]	PROGMEM = "DEMO";
#endif
#ifdef _APP_LINES_H_
extern const char menu_str_lines[]	PROGMEM = "LINES";
#endif
extern const char menu_str_newsd[]	PROGMEM = "NEWSD";
extern const char menu_str_stats[]	PROGMEM = "STATS";

const char menu_str_apps[]	PROGMEM = "APPS";	// Go to Menu 1 with all the apps;
const char menu_str_alarm[]	PROGMEM = "ALARM";
const char menu_str_al_p[]	PROGMEM = "AL+";
const char menu_str_al_m[]	PROGMEM = "AL-";
const char menu_str_date_p[]	PROGMEM = "DATE+";
const char menu_str_date_m[]	PROGMEM = "DATE-";
const char menu_str_remi_p[]	PROGMEM = "REMI+";
const char menu_str_remi_m[]	PROGMEM = "REMI-";
const char menu_str_temp_p[]	PROGMEM = "TEMP+";	// (fc, Jan 30/2011) show or not the temperature;
const char menu_str_temp_m[]	PROGMEM = "TEMP-";
const char menu_str_mesg_p[]	PROGMEM = "MESG+";
const char menu_str_mesg_m[]	PROGMEM = "MESG-";
#ifdef WANT_TIME_AT_CITY
const char menu_str_city_p[]	PROGMEM = "CITY+";
const char menu_str_city_m[]	PROGMEM = "CITY-";
#endif
const char menu_str_chme_p[]	PROGMEM = "CHME+";	// (fc, Feb 3/2011) enable/disable chime;
const char menu_str_chme_m[]	PROGMEM = "CHME-";
#ifdef WANT_ANIMATIONS
const char menu_str_anim_p[]	PROGMEM = "ANIM+";	// animations on quarter hour
const char menu_str_anim_m[]	PROGMEM = "ANIM-";
#endif
const char menu_str_font_p[]	PROGMEM = "FONT+";	// Large Font
const char menu_str_font_m[]	PROGMEM = "FONT-";
#ifdef WANT_EVENT_LOG
const char menu_str_log_p[]	PROGMEM = "LOG+";	// log temperature on the hour
const char menu_str_log_m[]	PROGMEM = "LOG-";
const char menu_str_log_c[]	PROGMEM = "CLRLG";
#endif
const char menu_str_cels[]	PROGMEM = "CELS";	// Celsius
const char menu_str_fahr[]	PROGMEM = "FAHR";	// Fahrenheit
const char menu_str_24h_p[]	PROGMEM = "24H+";	//*	12/24 hour mode by <MLS> Apr 3, 2010
const char menu_str_24h_m[]	PROGMEM = "24H-";
const char menu_str_dst_p[]	PROGMEM = "DST+";	/* DST currently active */
const char menu_str_dst_m[]	PROGMEM = "DST-";	/* DST not active */
const char menu_str_time[]	PROGMEM = "TIME";	//*	time command added by <MLS> Apr 2, 2010
const char menu_str_ymd[]	PROGMEM = "Y M D";
const char menu_str_day[]	PROGMEM = "DAY";
const char menu_str_pwron[]	PROGMEM = "PWRON";	/* What to run at power on */

const char * menu[]		PROGMEM = {
	menu_str_setup,
#ifdef _APP_QUOTE_H_
	menu_str_quote,
#endif
#ifdef _APP_BIG_H_
	menu_str_big,
#endif
#ifdef _APP_TIX_H_
	menu_str_tix,
#endif
#ifdef _APP_WORDS_H_
	menu_str_words,
#endif
#ifdef _APP_CNTDOWN_H_
	menu_str_cntdn,
#endif
#ifdef _APP_STOPWATCH_H_
	menu_str_stopw,
#endif
#ifdef _APP_SCORE_H_
	menu_str_score,
#endif
#ifdef _APP_PONG_H_
	menu_str_pong,
#endif
#ifdef _APP_PACMAN_H_
	menu_str_pacmn,
#endif
#ifdef _APP_TIMELIVED_H_
	menu_str_lived,
#endif
#ifdef _APP_ANIMATION_H_
	menu_str_anim,
#endif
#ifdef _APP_TIMECLOCK_H_
	menu_str_tclok,
#endif
	menu_str_utc,
#ifdef _APP_SUN_H_
 #if NUM_DISPLAYS < 2
	menu_str_sun,
 #endif
#endif
#ifdef _APP_LIFE_H_
	menu_str_life,
#endif
#ifdef _APP_DEMO_H_
	menu_str_demo,
#endif
	menu_str_newsd,
	menu_str_stats,

	menu_str_apps,		menu_str_alarm,		menu_str_al_p,
	menu_str_al_m,		menu_str_date_p,	menu_str_date_m,
	menu_str_remi_p,	menu_str_remi_m,	menu_str_temp_p,
	menu_str_temp_m,	menu_str_mesg_p,	menu_str_mesg_m,
#ifdef WANT_TIME_AT_CITY
	menu_str_city_p,	menu_str_city_m,
#endif
	menu_str_chme_p,	menu_str_chme_m,
#ifdef WANT_ANIMATIONS
	menu_str_anim_p,	menu_str_anim_m,
#endif
	menu_str_font_p,	menu_str_font_m,
#ifdef WANT_EVENT_LOG
	menu_str_log_p,		menu_str_log_m,		menu_str_log_c,
#endif
	menu_str_cels,		menu_str_fahr,		menu_str_24h_p,
	menu_str_24h_m,		menu_str_dst_p,		menu_str_dst_m,
	menu_str_time,		menu_str_ymd,		menu_str_day,
	menu_str_pwron
};

// enum for fetchExtraInfo
enum 
{
	XTRA_MSG		=	0,		// show message
	XTRA_DATE,					// show date
	XTRA_REMIND,					// show daily reminder
	XTRA_TEMP,					// show temperature
#ifdef WANT_TIME_AT_CITY
	XTRA_CITY,					// show city time
#endif
	XTRA_LAST					// must be last item
};



//*********************************************************************************************************
// constructor - initialize all member variables;
//
WiseClock::WiseClock()
{
  showExtraMessages	=	0;
  crtBuffer[0]		=	0;
  resetCrtPos();
  crtColor		=	GREEN;

  // current item in the menu;
  item			=	-1;
  isSdCard		=	false;
  isLongTrunc		=	false;
  isLongComment		=	false;
  isLongOpen		=	false;
  orgFile[0]		=	0;
  isShortTrunc		=	false;
  isShortComment	=	false;
  isShortOpen		=	false;
  isMenuActive		=	false;
  isSettingHours	=	true;
  isDateEnabled		=	true;
  isMessageEnabled	=	false;
  isTemperatureEnabled	=	false;
  isChimeEnabled	=	false;

#ifdef _APP_ANIMATION_H_
  appAnimation.isAnimEnabled		=	false;
  appAnimation.isAnimOpen		=	false;
#endif

#ifdef WANT_EVENT_LOG
  isLogEnabled		=	false;
#endif

  // (fc, Feb 4/2011) used for setting the date;
  isSettingYear		=	false;		
  isSettingMonth	=	false;
  isSettingDay		=	true;			// when setting the date, start with the year;		

  DSTPlus		=	false;			// Day light Saving Time Increment 
  DSTMinus		=	false; 			// Day light Saving Time Decrement

  nSpeed		=	DEFAULT_SPEED;		// default speed makes for a 20ms delay;
  nBrightness		=	DEFAULT_BRIGHTNESS;	// default maximum brightness;
  snoozeMins		=	DEFAULT_SNOOZE_MINUTES;
  buttonsInUse 		= 	0;
  currentMessageType	=	0;
  reminderTriggerEnabled =	false;
  startUpAppQuote 	=	false;

#ifdef WANT_TIME_AT_CITY
  cityIndex		= 	0;
#endif

#ifdef SERIAL_MSG
  personalMsg[0]	=	0;
  personalMsg[1]	=	0;
  pmPos			=	1;
#endif

  nextRun		=	millis() + 2;
}


void WiseClock::initSerial()
{
#ifdef SERIAL_MSG
  #ifdef USE_SOFTWARE_SERIAL
    uint8_t tmp = 1<<JTD;		// Disable JTAG
    MCUCR = tmp;			// Disable JTAG
    MCUCR = tmp;			// Disable JTAG
    swSerial1.begin(9600);
    #ifdef _WiFly_H_
    wiFly.init();
    #endif
  #else
    Serial1.begin(9600);
  #endif
    checkMessageCmd = false;
#endif
}

//*********************************************************************************************************
void	WiseClock::initApp()
{
  int16_t seed = (alarmClock.second << 10) ^ (alarmClock.minute << 5) ^ (alarmClock.hour << 2) ^ alarmClock.dow;
  randomSeed(seed);

  startApp(readUserSetting(powerOnAppLoc), false);
}

//*********************************************************************************************************
boolean   WiseClock::initSdCard()
{
  // initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
  // breadboards.  use SPI_FULL_SPEED for better performance.
  isSdCard = sd.init(SPI_HALF_SPEED, SS_PIN);

  return isSdCard;
}

//*********************************************************************************************************
// open a file for a long duration.  Cache the filename so a successive opens turn
// into a rewind.
//
boolean   WiseClock::openLongFile(const char* filename)
{
  if (!isSdCard)
  {
    memset(crtBuffer, ' ', BLANK_PREFIX);
    strcpy_P(&crtBuffer[BLANK_PREFIX], SdFailMsg);
    resetCrtPos();
    return false;
  }

  if (isLongOpen && sdLong.isOpen()) {
    if (strcmp(filename, longFile) == 0) {
      sdLong.seekSet(0);
      return true;
    } else {
      strcpy(orgFile, longFile);
      longPos = tellLongFile();
      sdLong.close();
    }
  }
  sdLong = SdFile(filename, O_READ);
  isLongOpen = sdLong.isOpen();
  strcpy(longFile, filename);

  // REM: test for error;
  return(isLongOpen);
}

//*********************************************************************************************************
// close a long duration file.  Reopen a previous file if necessary.
//
void   WiseClock::closeLongFile()
{
    if (isLongOpen && sdLong.isOpen()) {
	sdLong.close();
    }
    if (orgFile[0] != 0 && openLongFile(orgFile)) {
	sdLong.seekSet(wiseClock.longPos);
	orgFile[0] = 0;
	isLongOpen = true;
    } else {
	isLongOpen = false;
    }
}

//*********************************************************************************************************
// open a file for a short duration. Cache the filename so successive opens turn
// into a rewind.  Expect the file to be opened, read, and closed/rewound without
// interruption.
boolean   WiseClock::openShortFile(const char* filename, uint8_t mode)
{
  if (!isSdCard)
  {
    memset(crtBuffer, ' ', BLANK_PREFIX);
    strcpy_P(&crtBuffer[BLANK_PREFIX], SdFailMsg);
    resetCrtPos();
    return false;
  }

  if (isShortOpen && sdShort.isOpen()) {
    if (strcmp(filename, shortFile) == 0) {
      sdShort.seekSet(0UL);
      return true;
    } else {
      sdShort.close();
    }
  }
  sdShort = SdFile(filename, mode);
  isShortOpen = sdShort.isOpen();
  strcpy(shortFile, filename);

  // REM: test for error;
  return(isShortOpen);
}

//*********************************************************************************************************
// close a short duration file.
//
void   WiseClock::closeShortFile()
{
    if (isShortOpen && sdShort.isOpen()) {
	sdShort.close();
    }
    isShortOpen = false;
}


//*********************************************************************************************************
// (fc, Jul 14/09)
// read, from a file on SD card, a whole line (ending with CR LF) into the CRT buffer;
//
boolean WiseClock::readLineFromLongSD(char* buffer, int maxLength)
{
  boolean result;
  int16_t cnt;

  if (!isSdCard)
  {
    memset(crtBuffer, ' ', BLANK_PREFIX);
    strcpy_P(&crtBuffer[BLANK_PREFIX], SdFailMsg);
    resetCrtPos();
    return false;
  }

  cnt = sdLong.fgets(buffer, maxLength);
  if (!isLongTrunc) isLongComment = (buffer[0] == '#');
  isLongTrunc = false;
  if (cnt <= 0) {
    initCrtBuf();
    result = false;
  } else {
    result = true;
    cnt--;
    if (buffer[cnt] == '\n') {
      buffer[cnt] = 0;
    } else {
      isLongTrunc = true;
    }
  }

#ifdef _DEBUG_
  Serial.println(buffer);
  Serial.print("Is line a comment? ");
  Serial.println(isLongComment, DEC);
  Serial.print("getLine from quote1.txt failed? ");
  Serial.println(result, DEC);
#endif

    return result;
}

//*********************************************************************************************************
// read block of bytes from a file on SD card

int16_t WiseClock::readFromLongSD(char *buf, int16_t len)
{
    return(sdLong.read(buf, len));
}

//*********************************************************************************************************
// seek within file on SD card

boolean WiseClock::seekLongFile(uint32_t pos)
{
	return(sdLong.seekSet(pos));
}

//*********************************************************************************************************
// tell current position in file on SD card

uint32_t WiseClock::tellLongFile()
{
	return(sdLong.curPosition());
}

//*********************************************************************************************************
// (fc, Jul 14/09)
// read, from a file on SD card, a whole line (ending with CR LF) into the message buffer;
//
boolean WiseClock::readLineFromShortSD(char* buffer, int maxLength)
{
  boolean result;
  int16_t cnt;

  cnt = sdShort.fgets(buffer, maxLength);
  if (!isShortTrunc) isShortComment = (buffer[0] == '#');
  isShortTrunc = false;
  if (cnt <= 0) {
    result = false;
  } else {
    result = true;
    if (buffer[--cnt] == '\n') {
      buffer[cnt] = 0;
    } else {
      isShortTrunc = true;
    }
  }

#ifdef _DEBUG_
  Serial.println(buffer);
  Serial.print("getLine from message.txt failed? ");
  Serial.println(result, DEC);
#endif

  return result;
}

//*********************************************************************************************************
int16_t WiseClock::runappNewSD()
{
    if (appState) {
	clearDisplay();
	initApp();
	appState = 0;
	return(1);
    }
    displayStaticLine_P(PSTR("Pleas"), 0, ORANGE);
    displayStaticLine_P(PSTR("Wait "), 8, ORANGE);

    ht1632_plot(15, 15, RED);					// signal update mode;
    orgFile[0] = 0;
    closeLongFile();
    closeShortFile();
    initSdCard();
    initMessageFromSDcard();
    initCrtBuf();
    ht1632_plot(15, 15, BLACK);					// signal update mode;
    appState = 1;
    return(1000);
}

#ifdef WANT_EVENT_LOG
//*********************************************************************************************************
byte WiseClock::updateLogFile(char* type, int hour, int minute, int day, int month, int year, long p1, long p2)
{
	char tmpBuf[75];						// must be able to contain the complete log record;
	byte err, color;

	if (wiseClock.isLogEnabled == false)
		return 0;
		
	color = get_shadowram(15, 15);
	ht1632_plot(15, 15, RED);						// signal update mode;
		
	err = 0;
	openShortFile("WCLOG.CSV", O_WRITE|O_APPEND);
	sprintf_P(tmpBuf, PSTR("%s,%d,%d,%d,%d,%d,%ld,%ld%c%c"), type, hour, minute, day, month, year, p1, p2, 0xd, 0xa);
	sdShort.write(tmpBuf);
	if (sdShort.writeError) {
		displayStaticLine_P(PSTR("Write"), 0, RED);
		displayStaticLine_P(PSTR("SDErr"), 8, RED);
		err = 3;
		color = BLACK;
		delay(2000);
		clearDisplay();
	}
	closeShortFile();
	ht1632_plot(15, 15, color);						// signal end update mode;
	return err;	
}

//*********************************************************************************************************

int16_t WiseClock::runappLogClear()
{
	int16_t ms = 100;

	if (appState)
	{
		clearDisplay();
		initApp();
		appState = 0;
		return(1);
	}
	buttonsInUse = SET_BUTTON + PLUS_BUTTON;
	if (plusButtonCnt > 1)
		plusButtonCnt = 0;
	displayStaticLine_P(PSTR("Sure?"), 0, GREEN);
	if (plusButtonCnt == 0)
		displayStaticLine_P(PSTR(" No  "), 8, GREEN);
	else
		displayStaticLine_P(PSTR(" Yes "), 8, RED);

	if (setButtonCnt == 0)						// wait till Set button pressed;
		return(ms);

	if (plusButtonCnt > 0)
	{
		displayStaticLine_P(PSTR("Pleas"), 0, ORANGE);
		displayStaticLine_P(PSTR("Wait "), 8, ORANGE);

		ht1632_plot(15, 15, RED);					// signal update mode;
		closeShortFile();

		sdShort = SdFile("WCLOG.CSV" , O_WRITE|O_TRUNC|O_CREAT);
		if (sdShort.isOpen())
		{
			sdShort.close();
			ms = 1000;
		}
		else
		{
			displayStaticLine_P(PSTR("Open "), 0, RED);			// show error;
			displayStaticLine_P(PSTR("SDErr"), 8, RED);
			ms = 2000;
		}
		ht1632_plot(15, 15, BLACK);					// signal update mode;
		appState = 1;
	}
	plusButtonCnt = 0;
	buttonsInUse = 0;
	return(ms);
}
#endif

//*********************************************************************************************************
// set the application to run at power-on
//
int16_t WiseClock::runappSetPowerOn()
{
	buttonsInUse = SET_BUTTON + PLUS_BUTTON;
	if (plusButtonCnt > MAX_ITEM_MENU1)
		plusButtonCnt = MENU_SETUP + 1;
	if (plusButtonCnt <= MENU_SETUP)
		plusButtonCnt = MENU_SETUP + 1;

	if (appState != plusButtonCnt)
	{
		// display the menu item;
		strcpy_P(tempBuffer, (char *)pgm_read_word(&(menu[MENU_SET_POWER_ON])));
		displayStaticLine(tempBuffer, 0, GREEN);
		clearLine(8);
		strcpy_P(tempBuffer, (char *)pgm_read_word(&(menu[plusButtonCnt])));
		displayStaticLine(tempBuffer, 8, ORANGE);
		appState = plusButtonCnt;
	}

	// wait till Set button pressed;
	if (setButtonCnt == 0)
		return(100);

	buttonsInUse = 0;
	plusButtonCnt = 0;
	saveUserSetting(powerOnAppLoc, appState); 
	startApp(appState, false);
	return(1);
}

//*********************************************************************************************************
// start up the desired application
//
void WiseClock::startApp(byte item, boolean setup)
{

	if (item > MAX_ITEM_MENU1 || item <= MENU_SETUP)
		item = MENU_SETUP + 1;
	// action depends on the menu item;

	saveCrtApp();

	switch (item)
	{
#ifdef _APP_QUOTE_H_
		case MENU_QUOTE:
			if (setup) buttonsInUse	=	SET_BUTTON + PLUS_BUTTON;
			// (fc, Mar 28/2012);
			crtApp		=	APP_QUOTE;
			pCrtApp = &appQuote;
			pCrtApp->init();
			startUpAppQuote =	setup;
			crtPos		=	9999;		// force a fresh start;
			break;
#endif

#ifdef _APP_BIG_H_
		case MENU_BIG:
			crtApp		=	APP_BIG;
			pCrtApp = &appBig;
			pCrtApp->init(appBig.bigStartMode);
			crtPos          =	9999;		// force a fresh start;
			break;
#endif

#ifdef _APP_WORDS_H_
		case MENU_WORDS:
			crtApp		=	APP_WORDS;
			pCrtApp = &appWords;
			pCrtApp->init(setup);
			break;
#endif

#ifdef _APP_TIX_H_
		case MENU_TIX:
			crtApp		=	APP_TIX;
			pCrtApp = &appTix;
			pCrtApp->init(setup);
			break;
#endif

#ifdef _APP_STOPWATCH_H_
		case MENU_STOPW:
			crtApp		=	APP_STOPW;
			pCrtApp = &appStopWatch;
			pCrtApp->init();
			break;
#endif

#ifdef _APP_TIMECLOCK_H_
		case MENU_TCLOK:
			crtApp		=	APP_TCLOK;
			pCrtApp = &appTimeClock;
			pCrtApp->init();
			break;
#endif

#ifdef _APP_CNTDOWN_H_
		case MENU_CNT_DOWN:
			crtApp		=	APP_CNT_DOWN;
			pCrtApp = &appCntDown;
			pCrtApp->init(setup);	// (fc, Sep 14, 2013) resume counting down after power is restored;
			break;
#endif

#ifdef _APP_SCORE_H_
		case MENU_SCORE:
			crtApp		=	APP_SCORE;
			pCrtApp = &appScore;
			pCrtApp->init(setup);	// (fc, Sep 21, 2013) resume score after power is restored;
			break;
#endif

#ifdef _APP_PONG_H_
		case MENU_PONG:
			crtApp		=	APP_PONG;
			pCrtApp = &appPong;
			pCrtApp->init();
			break;
#endif
			
#ifdef _APP_PACMAN_H_
		case MENU_PAC:
			crtApp		=	APP_PACMAN;
			pCrtApp = &appPacman;
			pCrtApp->init();
			break;
#endif

#ifdef _APP_ANIMATION_H_
		case MENU_ANIM:
			crtApp		=	APP_ANIM;
			pCrtApp = &appAnimation;
			pCrtApp->init();
			break;
#endif
#ifdef _APP_TIMELIVED_H_
		case MENU_LIVED:
			crtApp		=	APP_LIVED;
			pCrtApp = &appTimeLived;
			pCrtApp->init();
			break;
#endif

#ifdef _APP_UTC_H_
		case MENU_UTC:
			crtApp		=	APP_UTC;
			pCrtApp = &appUtc;
			pCrtApp->init(setup);
			break;
#endif

#ifdef _APP_SUN_H_
 #if NUM_DISPLAYS < 2
		case MENU_SUN:
			crtApp		=	APP_SUN;
			pCrtApp = &appSun;
			pCrtApp->init();
			break;
 #endif
#endif

#ifdef _APP_LIFE_H_
		case MENU_LIFE:
			crtApp		=	APP_LIFE;
			pCrtApp = &appLife;
			// use the current display as the starting point;
			pCrtApp->init(setup);
			break;
#endif

#ifdef _APP_DEMO_H_
		case MENU_DEMO:
			crtApp		=	APP_DEMO;
			pCrtApp = &appDemo;
			pCrtApp->init(setup);
			crtPos		=	9999;	// force a fresh start;
			break;
#endif

		case MENU_NEWSD:
			crtApp 		= 	APP_NEWSD;
			appState	=	0;
			break;
			

		case MENU_STATS:
			crtApp 		= 	APP_STATS;
			setUpRepeat 	= 	0;		
			crtPos		=	9999;	// force a fresh start;
			break;
	}
}


//*********************************************************************************************************
// The message file is always read from beginning to end.
// The sequence "JAN1" followed by "JAN1,2015" or v.v. will be handled correctly 
// Dates for DST are independant from dates for the reminders
//
boolean WiseClock::getMessageAndReminder(byte pmNumber)
{
    char *Ptr1, *Ptr2, color;
    int i;
    boolean yearIsSpecified;
    boolean DSTSet;
    char tmpBuffer[30];
	
    reminderMsg[0] = 0x0;				// no reminder message yet;
    DSTPlus = false;
    DSTMinus = false;
    DSTSet = false;

    if (!openShortFile("message.txt", O_READ))
	return false;

    color = get_shadowram(15, 15);
    ht1632_plot	(15, 15, ORANGE);		// Show start of get reminder processing

#ifdef WANT_TIME_AT_CITY
    for(i=0; i<10; i++) {
	city[i].name[0] = 0;
	city[i].utc = 0;
 #ifdef _APP_SUN_H_				// city lat, lon only useful if we have a map
	city[i].lat = 0;
	city[i].lon = 0;
 #endif
	city[i].dst_start = 0;
	city[i].dst_end = 0;
    }
#endif

#ifdef _WiFly_H_
    wiFly.configInit();
#endif
	
    while (true)
    {
	yearIsSpecified = false;

	while (isShortTrunc)
	    readLineFromShortSD(tmpBuffer, sizeof(tmpBuffer)-1);
	if (!readLineFromShortSD(tmpBuffer, sizeof(tmpBuffer)-1))			// end of file or no file ?
	{
#ifdef WANT_TIME_AT_CITY
	    if (city[cityIndex].name[0] == 0)
		cityIndex = 0;
#endif
	    closeShortFile();
#ifdef _WiFly_H_
	    wiFly.config();
#endif
	    ht1632_plot	(15, 15, color);
	    return false;
	}	
	if (isShortComment)
	    continue;
			
#ifndef SERIAL_MSG
	// personal message processing
	Ptr2 = tmpBuffer;	
	Ptr1 = personalMsg;
	if ((*Ptr2 == '[') && (*++Ptr2 == 'M'))		// is it [M?] ;
	{
	    if ((*++Ptr2 == '0' + pmNumber) && (*++Ptr2 == ']'))
	    {
		// copy personal message;
		Ptr2 += 2;
		i = strlen(Ptr2);
		memmove(Ptr1, Ptr2, i+1);	// copy null at end
		if (isShortTrunc)
		    readLineFromShortSD(&Ptr1[i], MAX_SMSG_LEN - i);
	    }
	    continue;
	}	
#endif
	
	// reminder message processing
	Ptr2 = tmpBuffer;	
	Ptr1 = monthName[alarmClock.month];
	for (i=0; i < 3; i++)
	{
	    if (((*Ptr1++) & 0x5F) != ((*Ptr2++) & 0x5F))				// convert to upper case, then compare
		break;
	}		

	if (i == 3)									// Is Month equal;
	{
	    if (*Ptr2 == '0')
		++Ptr2;								// skip pre zero;
	    if ((alarmClock.day) == atoi(Ptr2))  	// Is Day equal;  
	    {
		++Ptr2;
		if (alarmClock.day > 9)				// 2 digits ?;
		    ++Ptr2;
		if (*Ptr2++ == ',')					// is it "Jan25,20xx ";
		{
		    if (alarmClock.year != atoi(Ptr2))  // is Year equal;
			continue;
		    Ptr2 += 5;						// skip year;
		    yearIsSpecified = true;	
		}

		Ptr1 = Ptr2;						// save start of reminder message;
		if ((*Ptr2 == 'D') && (*(Ptr2 + 1) == 'S') && (*(Ptr2 + 2) == 'T'))
		{
		    if (*(Ptr2 + 3) == '+')  		// Is it DST+ ?;
		    {
			if (yearIsSpecified)		// always set DST if year specified
			{
			    DSTPlus = true;
			    DSTSet = true;
			}	
			if (!DSTSet)	
			    DSTPlus = true;	
			continue;
		    }	
		    if (*(Ptr2 + 3) == '-')  		//Is it DST- ?;
		    {
			if (yearIsSpecified)
			{
			    DSTMinus = true;
			    DSTSet = true;
			}	
			if (!DSTSet)	
			    DSTMinus = true;	
			continue;
		    }
		}
		if ((yearIsSpecified) || (reminderMsg[0] == 0x0))
		{
		    reminderMsg[0] = ' ';			// start with 1 space;
		    Ptr2 = &reminderMsg[1]; 

		    // copy new reminder without the Date;
		    i = strlen(Ptr1);
		    memmove(Ptr2, Ptr1, i+1);		// copy null at end
		    if (isShortTrunc)
			readLineFromShortSD(&Ptr2[i], (MAX_SMSG_LEN-1) - i);

		    continue;
		}	
	    }	
	}
#ifdef _APP_TIMELIVED_H_
	// APP_LIVED code
	// look for the **:**:**Z****-**-** pattern;
	if (
	    tmpBuffer[2] == ':'
	&&  tmpBuffer[5] == ':'
	&&  tmpBuffer[8] == 'Z'
	&&  tmpBuffer[13] == '-'
	&&  tmpBuffer[16] == '-'
	)
	{
	    tm birthDatetime = {0};
#ifdef _DEBUG_
	    // found the time and date;
	    Serial.print("Found birthday time and date: ");
	    Serial.println(tmpBuffer);
#endif
	    // REM: assume all are digits for now;
	    birthDatetime.hour   = 10*(tmpBuffer[0]-'0') + (tmpBuffer[1]-'0');
	    birthDatetime.minute = 10*(tmpBuffer[3]-'0') + (tmpBuffer[4]-'0');
	    birthDatetime.second = 10*(tmpBuffer[6]-'0') + (tmpBuffer[7]-'0');
	    birthDatetime.year   = 1000*(tmpBuffer[9]-'0') + 100*(tmpBuffer[10]-'0') + 10*(tmpBuffer[11]-'0') + (tmpBuffer[12]-'0');
	    birthDatetime.month  = 10*(tmpBuffer[14]-'0') + (tmpBuffer[15]-'0');
	    birthDatetime.day    = 10*(tmpBuffer[17]-'0') + (tmpBuffer[18]-'0');

	    appTimeLived.setBirthdate(birthDatetime);
	}
#endif
#ifdef WANT_TIME_AT_CITY
	// city processing -- look for C[#]. prefix
	Ptr2 = tmpBuffer;	
	if (
	   ((Ptr2[0] & 0x5F) == 'C') && (Ptr2[1] == '[')
	&& (Ptr2[2] >= '0') && (Ptr2[2] <= '9')
	&& (Ptr2[3] == ']') && (Ptr2[4] == '.')
	) {
	    byte ci = Ptr2[2] - '0';
	    Ptr2 += 5;
	    if ((Ptr2[0] & 0x5F) == 'N') {						// name
		for(Ptr2++; *Ptr2 && (*Ptr2 != ' ') && (*Ptr2 != '\t'); Ptr2++);	// skip to white space
		for(; *Ptr2 && (*Ptr2 == ' ' || *Ptr2 == '\t'); Ptr2++);		// skip white space
		Ptr1 = city[ci].name;
		strncpy(Ptr1, Ptr2, MAX_CITY_LEN);
		Ptr1[MAX_CITY_LEN] = 0;
	    } else if ((Ptr2[0] & 0x5F) == 'U') {					// UTC offset
		for(Ptr2++; *Ptr2 && (*Ptr2 != ' ') && (*Ptr2 != '\t'); Ptr2++);	// skip to white space
		for(; *Ptr2 && (*Ptr2 == ' ' || *Ptr2 == '\t'); Ptr2++);		// skip white space
		city[ci].utc = atoi(Ptr2) * 2;
		for(Ptr2++; *Ptr2 && *Ptr2 <= '0' && *Ptr2 >= '9'; Ptr2++);		// skip to white space
		if (Ptr2[0] == '.' && Ptr2[1] == '5') {					// handle half-hour offsets
		    if (city[ci].utc >= 0)
			city[ci].utc++;
		    else
			city[ci].utc--;
		}
 #ifdef _APP_SUN_H_	// city lat, lon only useful if we have a map
	    } else if (((Ptr2[0] & 0x5F) == 'L') && ((Ptr2[1] & 0x5F) == 'A')) {	// latitude
		for(Ptr2++; *Ptr2 && (*Ptr2 != ' ') && (*Ptr2 != '\t'); Ptr2++);	// skip to white space
		for(; *Ptr2 && (*Ptr2 == ' ' || *Ptr2 == '\t'); Ptr2++);		// skip white space
		city[ci].lat = atoi(Ptr2);
	    } else if (((Ptr2[0] & 0x5F) == 'L') && ((Ptr2[1] & 0x5F) == 'O')) {	// longitude
		for(Ptr2++; *Ptr2 && (*Ptr2 != ' ') && (*Ptr2 != '\t'); Ptr2++);	// skip to white space
		for(; *Ptr2 && (*Ptr2 == ' ' || *Ptr2 == '\t'); Ptr2++);		// skip white space
		city[ci].lon = atoi(Ptr2);
 #endif
	    } else if (((Ptr2[0] & 0x5F) == 'D') && ((Ptr2[3] == '+') || (Ptr2[3] == '-'))) {	// DST
		boolean pm = (Ptr2[3] == '+');
		int8_t d, m;
		int16_t jd;

		for(Ptr2++; *Ptr2 && (*Ptr2 != ' ') && (*Ptr2 != '\t'); Ptr2++);	// skip to white space
		for(; *Ptr2 && (*Ptr2 == ' ' || *Ptr2 == '\t'); Ptr2++);		// skip white space

		Ptr2[0] &= 0x5F;
		Ptr2[1] &= 0x5F;
		Ptr2[2] &= 0x5F;
		for(m=1; m<13; m++) {
		    Ptr1 = monthName[m];
		    // convert to upper case, then compare
		    if (
		       ((Ptr1[0] & 0x5F) == Ptr2[0])
		    && ((Ptr1[1] & 0x5F) == Ptr2[1])
		    && ((Ptr1[2] & 0x5F) == Ptr2[2])
		    ) {
			break;
		    }
		}
		if (m > 12)								// valid month not found
		    continue;

		Ptr2 += 3;
		if (*Ptr2 == '0')
		    Ptr2++;								// skip pre zero;
		d = atoi(Ptr2);
		Ptr2++;
		if (d > 9) Ptr2++;							// 2 digits ?
		if (*Ptr2++ == ',') {							// is it "Jan25,20xx ";
		    if (alarmClock.year != atoi(Ptr2))					// is Year equal;
			continue;
		}
		jd = julian_day(alarmClock.year, m, d);
		if (pm) city[ci].dst_start = jd;
		else city[ci].dst_end = jd;
	    }
	}
#endif
#ifdef _WiFly_H_
	wiFly.parseConfig(tmpBuffer);
#endif
    }
}


//*********************************************************************************************************
// (fc, Apr 15/2010)
boolean WiseClock::isLineInProgress()
{
  if (isLongTrunc && (crtPos > MAX_MSG_LEN - 32)) {
    int i = crtLen - crtPos;
    if (i > 0)
      memmove(crtBuffer, &crtBuffer[crtPos], i+1);	// copy null at end
    else
      i = 0;
    readLineFromLongSD(&crtBuffer[i], MAX_MSG_LEN - i);
    resetCrtPos();
  }

  return (crtPos < crtLen);
}

//*********************************************************************************************************
// called from setup();
//
void WiseClock::initCrtBuf()
{
#ifdef _APP_QUOTE_H_
	strcpy_P(crtBuffer, PSTR("quot1.txt"));
	crtBuffer[4]	=	'0' + appQuote.getQuoteFileDigit();
	openLongFile(crtBuffer);
#endif
	// current line string is built by copying each character to the position of the pointer, then advancing the pointer;
	crtBuffer[0]	=	0;
	resetCrtPos();
}

//*********************************************************************************************************
// Reset the CrtBuf position and length
//
void WiseClock::resetCrtPos()
{
	crtPos = 0;
	crtLen = strlen(crtBuffer);
	scrPos = 0;
	nextScroll = millis() + 2;
}

//*********************************************************************************************************
// Check for (and execute) any pending scroll operation
//
void WiseClock::checkScroll(unsigned long ms)
{
    if (isLineInProgress() && ((long)(ms - nextScroll) >= 0)) {
	if (largeTextFont) {
	    if (scrPos <= -12) {
		crtPos++;
		scrPos = 0;
	    }
	    displayLargeScrollingLine();
	    scrPos--;
	} else {
	    if (scrPos >= 6) {
		crtPos++;
		scrPos = 0;
	    }
	    displaySmallScrollingLine();
	    scrPos++;
	}
	nextScroll = ms + 53 - (nSpeed * 8);
    }
}

//*********************************************************************************************************
// Read any pending serial bytes
//
void WiseClock::checkSerial()
{
#ifdef SERIAL_MSG
    char c;

  #ifdef USE_SOFTWARE_SERIAL
    while(swSerial1.available() > 0) {
	c = swSerial1.read();
  #else
    while(Serial1.available() > 0) {
	c = Serial1.read();
  #endif
	if (c == '\r' || c == '\n') {			// if CR or LF (end-of-line)
	    checkMessageCmd = true;
	    pmPos = 1;					// get ready for another message
	} else if (pmPos < MAX_SMSG_LEN-1) {
	    if (pmPos == 1)
		personalMsg[0] = 0;			// flag new message as in-progress
	    personalMsg[pmPos++] = c;			// add character to buffer
	    personalMsg[pmPos] = 0;			// keep string null-terminated
	}
    }
#ifdef _WiFly_H_
    wiFly.check();
#endif

    if (checkMessageCmd) {
	checkMessageCmd = false;
	if (
	   ((personalMsg[1] & 0x5F) == 'C')		// if message is "CLS", clear the message.
	&& ((personalMsg[2] & 0X5F) == 'L')
	&& ((personalMsg[3] & 0x5F) == 'S')
	) {
	    personalMsg[0] = 0;				// clear message by setting flag that message is invalid
	    personalMsg[1] = 0;
	}
	else if ((strncasecmp_P(&personalMsg[1], PSTR("SET TIME = "), 11) == 0)
	&&       (personalMsg[14] == ':')
	&&       (personalMsg[17] == ':')
	) {
	    int8_t hr = atoi(&personalMsg[12]);
	    int8_t mn = atoi(&personalMsg[15]);
	    int8_t sc = atoi(&personalMsg[18]);
	    if (hr >= 0 && hr <= 23 && mn >= 0 && mn <= 59 && sc >= 0 && sc <= 59) {
		alarmClock.setTime(hr, mn, sc, alarmClock.year, alarmClock.month, alarmClock.day, alarmClock.dow);
	    }
	}
	else if ((strncasecmp_P(&personalMsg[1], PSTR("SET ALARM = "), 12) == 0)
	&&       (personalMsg[15] == ':')
	) {
	    int8_t hr = atoi(&personalMsg[13]);
	    int8_t mn = atoi(&personalMsg[16]);
	    if (hr >= 0 && hr <= 23 && mn >= 0 && mn <= 59) {
		alarmClock.alarmHour = hr;
		alarmClock.alarmMin = mn;
		saveAlarmTime();
	    }
	}
	else if ((strncasecmp_P(&personalMsg[1], PSTR("ENABLE ALARM"), 12) == 0)
	&&       (personalMsg[13] == 0)
	) {
	    alarmClock.isAlarmEnabled = true;
	    saveAlarmOnOff();
	}
	else if ((strncasecmp_P(&personalMsg[1], PSTR("DISABLE ALARM"), 13) == 0)
	&&       (personalMsg[14] == 0)
	) {
	    alarmClock.isAlarmEnabled = false;
	    saveAlarmOnOff();
	}
	else if ((strncasecmp_P(&personalMsg[1], PSTR("SILENCE ALARM"), 13) == 0)
	&&       (personalMsg[14] == 0)
	) {
	    silenceAlarm();
	}
	else if (personalMsg[1] != 0) {			// message not a command, so mark as valid for display
	    personalMsg[0] = ' ';			// message valid, start with an extra blank
	}
    }
#endif
}



//*********************************************************************************************************
void WiseClock::fetchDate()
{
	memset(crtBuffer, ' ', BLANK_PREFIX);
	sprintf_P(&crtBuffer[BLANK_PREFIX], PSTR("%s %s %d, %d    "), dayName[alarmClock.dow], monthName[alarmClock.month], alarmClock.day, alarmClock.year);
	// other date formats may be selected through the menu;
	//	sprintf(dateLine, "         %s-%d/%d/%d    ", dayName[alarmClock.dow], alarmClock.day, alarmClock.month, alarmClock.year);
	
#ifdef _DEBUG_
	Serial.println(crtBuffer);
#endif

	resetCrtPos();
	crtColor	=	ORANGE;
}


//*********************************************************************************************************
//
void WiseClock::splitTemp(int temp, int* pITemp, int* pDTemp, boolean useC)
{
	if (useC == false)
		temp = (temp * 9) / 5 + 128;			 // scaled by 4 , 128 = 4 x 32;
	*pITemp = temp / 4;
	*pDTemp = ((temp - (*pITemp * 4)) * 10) / 4;		// we want 1 decimal;
}


//*********************************************************************************************************
// (fc, Jan 30/2011);
void WiseClock::fetchTemperature()
{
	int16_t temp, iTemp, dTemp, i2Temp, d2Temp;
	char s;

	temp = appTemperature.getTemperature();			// get Celsius temperature scaled by 4;

#ifdef _DEBUG_
	Serial.println();
	Serial.print("Temperature=");
	Serial.println(temp);
#endif

//	%5.2f format for float does not work; kludge required;
//	sprintf(buffer, "       Temp is %5.2fC    ", temp);

	splitTemp(temp, &iTemp, &dTemp, appTemperature.useCelsius);
	s = appTemperature.useCelsius ? 'C' : 'F';

	memset(crtBuffer, ' ', BLANK_PREFIX);
	sprintf_P(&crtBuffer[BLANK_PREFIX], PSTR("Temp is %d.%d%c%c  "), iTemp, dTemp, 0x7f, s);  // chr(0x7f) = degree symbol;
		
	splitTemp(alarmClock.high24Temp, &iTemp, &dTemp, appTemperature.useCelsius);

	if (iTemp < 240)	// 240 is 60 C scaled by 4 (140 F)
	{
		splitTemp(alarmClock.low24Temp, &i2Temp, &d2Temp, appTemperature.useCelsius);

		sprintf_P(&crtBuffer[strlen(crtBuffer)], PSTR(" Highest today: %d.%d%c%c at %d:%02d, Lowest today: %d.%d%c%c at %d:%02d  "),
			iTemp, dTemp, 0x7f, s, alarmClock.high24Hour, alarmClock.high24Minute, i2Temp, d2Temp, 0x7f, s, alarmClock.low24Hour, alarmClock.low24Minute);  // chr(0x7f) = degree symbol;
	}

	resetCrtPos();
	crtColor	= 	ORANGE;
}


//*********************************************************************************************************
// 
// display reminder between quotations;
//
void WiseClock::fetchReminder()
{
    // start line with 6 spaces (to make it appear from the right when scrolling);
    memset(crtBuffer, ' ', BLANK_PREFIX);
    strcpy(&crtBuffer[BLANK_PREFIX], reminderMsg);

#ifdef _DEBUG_
    Serial.print  ("Reminder message:  ");
    Serial.println(crtBuffer);
#endif

    resetCrtPos();
#ifdef _APP_QUOTE_H_
    crtColor	= RED;
#else
    crtColor	= ORANGE;
#endif
}



//*********************************************************************************************************
// (fc, Apr 15/2010)
// read personalized message from SD card; it will be displayed between quotations;
//
void WiseClock::fetchMessage()
{
	// start line with 6 spaces (to make it appear from the right when scrolling);
#ifdef SERIAL_MSG
	memset(crtBuffer, ' ', BLANK_PREFIX-1);
	strcpy(&crtBuffer[BLANK_PREFIX-1], personalMsg);
#else
	memset(crtBuffer, ' ', BLANK_PREFIX);
	sprintf_P(&crtBuffer[BLANK_PREFIX], PSTR("%c%s%c"), '"', personalMsg, '"');
#endif

#ifdef _DEBUG_
	Serial.print  ("Personal message:  ");
	Serial.println(crtBuffer);
#endif
	resetCrtPos();
	crtColor	=	RED;
}


//*********************************************************************************************************
// (fc, Apr 15/2010) read personalized message and reminder from SD card;
//   this is called only once, from setup();
//   note that the reminder needs to be checked (and eventually updated) when the day changes;
//
void    WiseClock::initMessageFromSDcard()
{
  if (isSdCard)
  {
    getMessageAndReminder(getMessageNumber());		// get personalized message and reminder for this day;
  }
}


#ifdef WANT_TIME_AT_CITY
//*********************************************************************************************************
// (mcm, Apr 30/2012)
// display local time for a city
//
void WiseClock::fetchCity()
{
    int8_t hr, utc, mn, cm, cd, cdow, dayinc;
    int16_t jd, dst_s, dst_e;
    char suffix;

    // get current julian day
    cm = alarmClock.month;
    cd = alarmClock.day;
    cdow = alarmClock.dow;
    jd = julian_day(alarmClock.year, cm, cd);
    dayinc = 0;
    // convert time to UTC
    hr = alarmClock.hour;
    if (utcDiff != 0)
	hr -= utcDiff;
    if (DSTactive)
	hr--;

    // convert UTC to local time
    utc = city[cityIndex].utc;
    mn = utc / 2;
    mn = utc - (mn*2);
    mn *= 30;
    mn += alarmClock.minute;
    if (mn < 0) {
	mn += 60;
	hr--;
    } else if (mn > 59) {
	mn -= 60;
	hr++;
    }
    hr += (utc / 2);
    if (hr < 0) {
	jd--;
	dayinc--;
    } else if (hr > 23) {
	jd++;
	dayinc++;
    }

    // compensate for local daylight savings time
    dst_s = city[cityIndex].dst_start;
    dst_e = city[cityIndex].dst_end;

    if (dst_s < dst_e) {			// northern hemisphere
	if (
	   (jd > dst_s && jd < dst_e)
	|| (jd == dst_s && hr >= 2)
	|| (jd == dst_e && hr < 3)
	) {
	    hr++;
	}
    } else if (dst_e < dst_s) {			// southern hemisphere
	if (
	   (jd > dst_s)
	|| (jd < dst_e)
	|| (jd == dst_s && hr >= 2)
	|| (jd == dst_e && hr < 3)
	) {
	    hr++;
	}
    }
    // align hour to 0 - 23
    if (hr < 0) {
	hr += 24;
	dayinc--;
    } else if (hr > 23) {
	hr -= 24;
	dayinc++;
    }
    if (dayinc > 0) {
	cdow++;
	if (cdow > 7) cdow = 1;
	cd++;
	if (cd > daysInMonth[cm - 1]) {
	    if (!((cd == 29) && (cm == 2) && ((alarmClock.year & 0x3) == 0) && (((alarmClock.year % 100) != 0) || ((alarmClock.year % 400) == 0)))) {
		cd = 1;
		cm++;
		if (cm > 12)
		    cm = 1;
	    }
	}
    } else if (dayinc < 0) {
	cdow--;
	if (cdow < 1) cdow = 7;
	cd--;
	if (cd <= 0) {
	    cm--;
	    if (cm < 1) {
		cm = 12;
		cd = 31;
	    } else {
		cd = daysInMonth[cm - 1];
		if ((cm == 2) && ((alarmClock.year & 0x3) == 0) && (((alarmClock.year % 100) != 0) || ((alarmClock.year % 400) == 0))) {
		    cd = 29;	// leap year has 29 days in February
		}
	    }
	}
    }

    memset(crtBuffer, ' ', BLANK_PREFIX);
    // handle AM/PM if necessary
    if (!alarmClock.is24hourModeEnabled) {
	if (hr >= 12) {
	    suffix = 'P';
	    hr -= 12;
	} else {
	    suffix = 'A';
	}
        if (hr == 0) hr = 12;
	sprintf_P(&crtBuffer[BLANK_PREFIX], PSTR("%s <G>%d:%02d %cM"), city[cityIndex].name, hr, mn, suffix);
    } else {
	sprintf_P(&crtBuffer[BLANK_PREFIX], PSTR("%s <G>%02d:%02d"), city[cityIndex].name, hr, mn);
    }
    jd = strlen(crtBuffer);
    sprintf_P(&crtBuffer[jd], PSTR(" %s %s %02d"), dayName[cdow], monthName[cm], cd);

    resetCrtPos();
    crtColor = ORANGE;

#ifdef WANT_TIME_AT_CITY
 #ifdef _APP_SUN_H_
  #if NUM_DISPLAYS > 1
    if (city[cityIndex].lat != 0 && city[cityIndex].lon != 0) {
	appSun.blinkLatLon(city[cityIndex].lat, city[cityIndex].lon);
    }
  #endif
 #endif
#endif

    // move to the next city
    cityIndex++;
    if ((cityIndex > 9) || (city[cityIndex].name[0] == 0))
	cityIndex = 0;
}
#endif


//*********************************************************************************************************
//*	(fc, Jan 30/2011)
void WiseClock::processSetDate()
{
	alarmClock.isDateSetting	=	true;

	// date format is YYMMDD;
	// move from YY, to MM, to DD, similar to time setting; blink the current selection;
	// press Set to move between day, month, year;
	if (isSettingDay)
	{
		isSettingDay = false;
		isSettingMonth = false;
		isSettingYear = true;
	}
	else if (isSettingMonth)
	{
		isSettingDay = true;
		isSettingMonth = false;
		isSettingYear = false;
	}
	else if (isSettingYear)
	{
		isSettingDay = false;
		isSettingMonth = true;
		isSettingYear = false;
	}

	clearLine(8);
	// show the current date;
	displayDateBlink();
}

//*********************************************************************************************************
// executed when Menu button is pressed;
// determines the menu item to be displayed;
//
void WiseClock::processButtonMenu()
{
	int8_t i;

	if (buttonsInUse & MENU_BUTTON)		// if Menu button in use inc counter;
	{
		++menuButtonCnt;
		return;
	}
	
	clearDisplay();
	buttonsInUse = 0;

	if (alarmClock.isTimeSetting || alarmClock.isAlarmSetting)
	{
		//*	this way it will go back to QUOTE when done setting the alarm or time
//		item	=	-1;

		alarmClock.isTimeSetting	=	false;
		alarmClock.isAlarmSetting	=	false;
	}
	else if (alarmClock.isDateSetting || alarmClock.isDaySetting)
	{
//        item    =   -1;
		alarmClock.setDateOnly();
		alarmClock.isDateSetting = false;
		alarmClock.isDaySetting	= false;
		reminderMsg[0] = 0;					// after date change no reminder set
		isSettingDay = true;				// always start with setting of year;
		isSettingMonth = false;
		isSettingYear = false;
	}

	isMenuActive	=	true;

	if (item == MAX_ITEM_MENU1)			
		item = MENU_SETUP - 1;
	if (item == MAX_ITEM_MENU2)			
		item = 	MENU_APPS - 1;

	item++;
	
	do
	{
		i = 0;
		switch (item)					// skip menu items if already set
		{
			case MENU_ALARM_ON:
				if (alarmClock.isAlarmEnabled)
					i = 1;
				break;
			case MENU_ALARM_OFF:
				if (!alarmClock.isAlarmEnabled)
					i = 1;
				break;
			case MENU_DATE_ON:
				if (isDateEnabled)
					i = 1;
				break;
			case MENU_DATE_OFF:
				if (!isDateEnabled)
					i = 1;
				break;
			case MENU_REMIND_ON:
				if (isReminderEnabled)
					i = 1;
				break;
			case MENU_REMIND_OFF:
				if (!isReminderEnabled)
					i = 1;
				break;
#ifdef WANT_TIME_AT_CITY
			case MENU_CITY_ON:
				if (isCityEnabled)
					i = 1;
				break;
			case MENU_CITY_OFF:
				if (!isCityEnabled)
					i = 1;
				break;
#endif
			case MENU_TEMP_ON:
				if (isTemperatureEnabled)
					i = 1;
				break;
			case MENU_TEMP_OFF:
				if (!isTemperatureEnabled)
					i = 1;
				break;
			case MENU_MSG_ON:
				if (isMessageEnabled)
					i = 1;
				break;
			case MENU_MSG_OFF:
				if (!isMessageEnabled)
					i = 1;
				break;
			case MENU_CHIME_ON:
				if (isChimeEnabled)
					i = 1;
				break;
			case MENU_CHIME_OFF:
				if (!isChimeEnabled)
					i = 1;
				break;
#ifdef WANT_ANIMATIONS
			case MENU_ANIM_ON:
				if (appAnimation.isAnimEnabled)
					i = 1;
				break;
			case MENU_ANIM_OFF:
				if (!appAnimation.isAnimEnabled)
					i = 1;
				break;
#endif
#ifdef WANT_EVENT_LOG
			case MENU_LOG_ON:
				if (isLogEnabled)
					i = 1;
				break;
			case MENU_LOG_OFF:
				if (!isLogEnabled)
					i = 1;
				break;
#endif
			case MENU_24_HOUR_ON:
				if (alarmClock.is24hourModeEnabled)
					i = 1;
				break;
			case MENU_24_HOUR_OFF:
				if (!alarmClock.is24hourModeEnabled)
					i = 1;
				break;
			case MENU_DST_ON:
				if (DSTactive)
					i = 1;
				break;
			case MENU_DST_OFF:
				if (!DSTactive)
					i = 1;
				break;
			case MENU_CELSIUS_ON:
				if (appTemperature.useCelsius)
					i = 1;
				break;
			case MENU_CELSIUS_OFF:
				if (!appTemperature.useCelsius)
					i = 1;
				break;
			case MENU_LFONT_ON:
				if (largeTextFont)
					i = 1;
				break;
			case MENU_LFONT_OFF:
				if (!largeTextFont)
					i = 1;
				break;
		}
		item = item + i;	
		if (item > MAX_ITEM_MENU2)
		{		
			item = MENU_APPS;
		}	

	} while (!i == 0);
	
	// clear the top part of the screen;
	crtPos = 9999;
	clearLine(0);

	// display the menu item;
	strcpy_P(tempBuffer, (char *)pgm_read_word(&(menu[item])));
	displayStaticLine(tempBuffer, 0, GREEN);
}


//*********************************************************************************************************
// executed when Set button is pressed;
// when pressed independently (not in menu), it increments dimming (then rolls over);
//
void WiseClock::processButtonSet()
{
	if (isMenuActive)
	{
		// action depends on the menu item;
		if (item == MENU_SETUP)
		{
			item		=	MENU_APPS;	// switch to Apps menu;	
			processButtonMenu();
			return;
		}
		// handle applications
		if (item > MENU_SETUP && item <= MAX_ITEM_MENU1)
		{
			startApp(item, true);
			--item;						
			isMenuActive	=	false;
			return;
		}

		saveCrtApp();

		// handle config options
		switch (item)
		{
			case MENU_APPS:
				item = MENU_SETUP;						// switch to Setup menu;
				processButtonMenu();
				break;
				
			case MENU_SET_ALARM:
				alarmClock.isAlarmSetting	=	true;
				// press Set to alternate between hours and minutes;
				isSettingHours	=	!isSettingHours;
				clearLine(8);
				// show the alarm time;
				displayTimeBlink(alarmClock.alarmHour, alarmClock.alarmMin, alarmClock.isAlarmEnabled);
				break;

			case MENU_ALARM_ON:
				// show that alarm is ON with dot ('.') between hours and minutes;
				alarmClock.isAlarmEnabled	=	true;
				saveAlarmOnOff();
				++item;					//skip Alarm off in menu;
				clearDisplay();
				isMenuActive	=	false;
				break;

			case MENU_ALARM_OFF:
				// show that alarm is OFF with colon (':') between hours and minutes;
				alarmClock.isAlarmEnabled	=	false;
				saveAlarmOnOff();
				clearDisplay();
				isMenuActive	=	false;
				break;

			case MENU_DATE_ON:
				// show date between quotes;
				isDateEnabled	=	true;
				saveShowDate();
				++item;					// skip Date off in menu;
				checkCrtApp();
				break;

			case MENU_DATE_OFF:
				// do not show date between quotes;
				isDateEnabled	=	false;
				saveShowDate();
				checkCrtApp();
				break;

			case MENU_REMIND_ON:
				// show Reminder between quotes;
				isReminderEnabled	=	true;
				saveShowReminder();
				++item;					//skip Reminder off in menu;
				checkCrtApp();
				break;

			case MENU_REMIND_OFF:
				// do not show Reminder between quotes;
				isReminderEnabled	=	false;
				saveShowReminder();
				checkCrtApp();
				break;

#ifdef WANT_TIME_AT_CITY
			case MENU_CITY_ON:
				// show city time between quotes;
				isCityEnabled	=	true;
				saveShowCity();
				++item;					//skip city off in menu;
				checkCrtApp();
				break;

			case MENU_CITY_OFF:
				// do not show city time between quotes;
				isCityEnabled	=	false;
				saveShowCity();
				checkCrtApp();
				break;
#endif

			case MENU_TEMP_ON:
				// show "temperature" message between quotes;
				isTemperatureEnabled	=	true;
				saveShowTemperature();
				++item;					//skip Temp off in menu;
				checkCrtApp();
				break;

			case MENU_TEMP_OFF:
				isTemperatureEnabled	=	false;
				saveShowTemperature();
				checkCrtApp();
				break;

			case MENU_MSG_ON:
#ifdef SERIAL_MSG
				isMessageEnabled	=	true;
				saveShowMessage();
				++item;					//skip MSG off in menu;
				checkCrtApp();
				break;

#else
				buttonsInUse	=	SET_BUTTON + PLUS_BUTTON;
				plusButtonCnt	=	getMessageNumber();
				setButtonCnt	=	0;
				crtApp		=	APP_MSG;
				clearLine(8);
				--item;
				isMenuActive	=	false;
#endif
				break;

			case MENU_MSG_OFF:
				isMessageEnabled	=	false;
				saveShowMessage();
				checkCrtApp();
				break;

			case MENU_CHIME_ON:
				isChimeEnabled	=	true;
				saveChime();
				++item;					//skip Chime off in menu;
				clearDisplay();
				isMenuActive	=	false;
				break;

			case MENU_CHIME_OFF:
				isChimeEnabled	=	false;
				saveChime();
				clearDisplay();
				isMenuActive	=	false;
				break;
#ifdef WANT_ANIMATIONS
			case MENU_ANIM_ON:
				appAnimation.isAnimEnabled	=	true;
				appAnimation.saveAnim();
				++item;					//skip Anim off in menu;
				clearDisplay();
				isMenuActive	=	false;
				break;

			case MENU_ANIM_OFF:
				appAnimation.isAnimEnabled	=	false;
				appAnimation.saveAnim();
				clearDisplay();
				isMenuActive	=	false;
				break;
#endif
#ifdef WANT_EVENT_LOG
			case MENU_LOG_ON:
				isLogEnabled	=	true;
				saveLog();
				++item;					//skip Log off in menu;
				clearDisplay();
				isMenuActive	=	false;
				break;

			case MENU_LOG_OFF:
				isLogEnabled	=	false;
				saveLog();
				clearDisplay();
				isMenuActive	=	false;
				break;

			case MENU_LOG_CLEAR:
				buttonsInUse	=	SET_BUTTON + PLUS_BUTTON;
				plusButtonCnt	=	0;
				crtApp		=	APP_LOG_CLEAR;
				appState	=	0;
				clearDisplay();
				isMenuActive	=	false;
				break;
#endif
				
			case MENU_24_HOUR_ON:
				alarmClock.is24hourModeEnabled	=	true;
				save24HourMode();
				++item;					//skip 24 Hour enabled off in menu;
				clearDisplay();
				isMenuActive	=	false;
				break;

			case MENU_24_HOUR_OFF:
				alarmClock.is24hourModeEnabled	=	false;
				save24HourMode();
				clearDisplay();
				isMenuActive	=	false;
				break;

			case MENU_LFONT_ON:
				largeTextFont	=	true;
				saveFontSize();
				++item;					//skip Large Font off in menu;
				checkCrtApp();
				break;

			case MENU_LFONT_OFF:
				largeTextFont	=	false;
				saveFontSize();
				checkCrtApp();
				break;

			case MENU_CELSIUS_ON:
				crtApp		=	APP_TMP;
				pCrtApp = &appTemperature;
				pCrtApp->init(true);
				isMenuActive = false;
				--item;
//				setTemp(true);
				break;

			case MENU_CELSIUS_OFF:
				crtApp		=	APP_TMP;
				pCrtApp = &appTemperature;
				pCrtApp->init(false);
				isMenuActive = false;
				--item;
//				setTemp(false);
				break;
				
			case MENU_DST_ON:
				DSTPlus		=	false;
				alarmClock.incDSTHour();                // Daylight Saving Time, increment the hour, 2 -> 3;
				DSTactive	=	true;
				saveDSTactive();

				++item;
				clearDisplay();
				isMenuActive	=	false;
				break;

			case MENU_DST_OFF:
				DSTMinus	=	false;
				alarmClock.decDSTHour();                // Daylight Saving Time, decrement the hour, 3 -> 2;
				DSTactive	=	false;
				saveDSTactive();

				clearDisplay();
				break;

			case MENU_SET_TIME:
				alarmClock.isTimeSetting	=	true;
				// press Set to alternate between hours and minutes;
				isSettingHours	=	!isSettingHours;
				clearLine(8);
				// show the current time;
				displayTimeBlink(alarmClock.hour, alarmClock.minute, alarmClock.isAlarmEnabled);
				break;
				
			case MENU_SET_DATE:
				processSetDate();
				break;

			case MENU_SET_DAY:
				alarmClock.isDaySetting		=	true;
				clearLine(8);
				// show the current day;
				displayDayBlink();
				break;

			case MENU_SET_POWER_ON:
				buttonsInUse	=	SET_BUTTON + PLUS_BUTTON;
				plusButtonCnt	=	readUserSetting(powerOnAppLoc);
				setButtonCnt	=	0;
				crtApp		=	APP_SET_POWER_ON;
				appState	=	item;
				clearDisplay();
				isMenuActive	=	false;
				break;

		}
	}
	else
	{
		if (buttonsInUse & SET_BUTTON)		// if Set button in use inc counter;
			++setButtonCnt;	
		else	
		{
			if (++nSpeed > MAX_SPEED)
			{
				nSpeed	=	0;
			}
			saveSpeed();
		}
	}	
}

//*********************************************************************************************************
// 
void WiseClock::checkCrtApp()
{
	if ((crtApp == APP_BIG)
#ifdef _APP_WORDS_H_
	||  (crtApp == APP_WORDS)
#endif
	) {
		clearDisplay();      			// Clear screen and do not change crtApp
	} else {
#ifdef _APP_QUOTE_H_
		crtApp		=	APP_QUOTE;
		pCrtApp = &appQuote;
#else
		crtApp  =   APP_BIG;
		pCrtApp = &appBig;
#endif
	}

	isMenuActive =	false;
}

//*********************************************************************************************************
// executed when Plus button is pressed;
// when pressed independently (not in menu), it increments scrolling speed (then rolls over);
//
void WiseClock::processButtonPlus()
{
	if (alarmClock.isTimeSetting)
	{
		alarmClock.incrementTimeSetting(isSettingHours);
		displayTimeBlink(alarmClock.hour, alarmClock.minute, alarmClock.isAlarmEnabled);
		
	}
	else if (alarmClock.isAlarmSetting)
	{
		alarmClock.incrementAlarmSetting(isSettingHours);
		displayTimeBlink(alarmClock.alarmHour, alarmClock.alarmMin, alarmClock.isAlarmEnabled);
		saveAlarmTime();
	}
	else if (alarmClock.isDateSetting)
	{
		if (isSettingYear)
			alarmClock.incrementYear();
		else if (isSettingMonth)
			alarmClock.incrementMonth();
		else if (isSettingDay)
			alarmClock.incrementDay();

		displayDateBlink();
	}
	else if (alarmClock.isDaySetting)
	{
		alarmClock.incrementDOW();
		displayDayBlink();
	}
	// (rp, Oct 2010);
    else
	{
		if (buttonsInUse & PLUS_BUTTON)
			++plusButtonCnt;
		else
		{
			if (++nBrightness > MAX_BRIGHTNESS)
			{
				nBrightness	=	0;
			}
			saveBrightness();
			setBrightness(nBrightness);
		}	
	}
}

//*********************************************************************************************************
/*
* This works equally well for both 16x24 and 8x32 matrices.
* (fc, Jan 30/2011) display line using the crt color;
*/
void WiseClock::displaySmallScrollingLine()
{
	int tmpPos;
	byte color, nc;

	while((color = colorSpec(crtPos)) <= ORANGE) {
		crtColor = color;
		crtPos += 3;
	}

	// shift the top half of the screen 6 times, one column at a time;
	tmpPos = crtPos;
	color = crtColor;
	int j = 0;
#ifdef MULTI_DISPLAY_SCROLL
	while(j <= 36 * NUM_DISPLAYS) {
#else
	while(j <= 36) {
#endif
		nc = colorSpec(tmpPos);
		if (nc <= ORANGE) {
			color = nc;
			tmpPos += 3;
			continue;
		}
		ht1632_putchar(j-scrPos, 0, (tmpPos < crtLen) ? crtBuffer[tmpPos++] : ' ', color);
		j += 6;
	}
}


//*********************************************************************************************************
// Display scrolling text with large 14 bit high x 2-11 bit wide proportional font
//
void WiseClock::displayLargeScrollingLine()
{
	int tmpPos, x;
	byte color, nc;

	while((color = colorSpec(crtPos)) <= ORANGE) {
		crtColor = color;
		crtPos += 3;
	}

	// shift the 14 bit high characters one character to the left, 1 character is 2 - 11 bits wide;
	x = scrPos;
	tmpPos = crtPos;
	color = crtColor;
	do {
		byte nc = colorSpec(tmpPos);
		if (nc <= ORANGE) {
			color = nc;
			tmpPos += 3;
			continue;
		}
		x = ht1632_putLargeChar(x, 1, (tmpPos < crtLen) ? crtBuffer[tmpPos++] : ' ', color);
	}	
#ifdef MULTI_DISPLAY_SCROLL
	while ((x <= X_MAX) && (x != 0));		// fill display dots and stop when 1 first char is moved out of the display;	
#else
	while ((x <= 32) && (x != 0));			// fill display with 32 dots and stop if 1 first char is moved out of the display;	
#endif
	if (x == 0) {					// we've moved a complete character
		scrPos = -12;
	}
}


//*********************************************************************************************************
// look for a color spec in the crtBuffer -- <R>, <O>, or <G>
//
byte WiseClock::colorSpec(int pos)
{
    if (pos > (crtLen-3)) return(127);	// color spec not found
    if (crtBuffer[pos] != '<' || crtBuffer[pos+2] != '>') return (127);

    byte dc = crtBuffer[pos+1];
    if (dc == 'R') return(RED);
    else if (dc == 'O') return(ORANGE);
    else if (dc == 'G') return(GREEN);
    return(127);	// color spec not found
}


#ifndef SERIAL_MSG
//*********************************************************************************************************
// called when user selects "Message On" from the menu;
// reads/updates the message from message.txt file;
//
int16_t WiseClock::runappMsg()
{
	if (plusButtonCnt > 10)						// Personal Message M1, M2, M3.....M0;
		plusButtonCnt = 1;

	ht1632_putchar(10, 8, 'M', ORANGE);
	ht1632_putchar(16, 8, '0' + (plusButtonCnt % 10), ORANGE);

	if (setButtonCnt == 0)						// wait till Set button pressed;
		return(100);
		
	buttonsInUse	= 	0;
	
	getMessageAndReminder(plusButtonCnt % 10);
	saveMessageNumber(plusButtonCnt % 10);

	plusButtonCnt = 0;
	
	restoreCrtApp();
	checkCrtApp();

	// show "personalized" message between quotes;
	isMessageEnabled	=	true;
	saveShowMessage();
	return(100);
}
#endif


//*********************************************************************************************************
//  get personalized message, date, reminder or temperature;
//
boolean WiseClock::fetchExtraInfo()
{
	uint8_t i = currentMessageType;

	do
	{
		if (++currentMessageType >= XTRA_LAST)
			currentMessageType = 0;

		switch(currentMessageType)
		{
			case XTRA_MSG:
				// display personalized message between quotation lines;
				if ((isMessageEnabled) && (personalMsg[0] != 0x0))
				{
					fetchMessage();  		// fills the text buffer with personalized message;
					return true;
				}
				break;
	
			case XTRA_DATE:
				// display date between quotation lines;
				if (isDateEnabled)
				{
					fetchDate();  			// fills the text buffer with the date;
					return true;
				}
				break;
		
			case XTRA_REMIND:
				// display reminder message;
				if ((isReminderEnabled) && (reminderMsg[0] != 0x0))
				{
					fetchReminder();  		// fills the text buffer with the reminder message;
					return true;
				}
				break;
			
			case XTRA_TEMP:
				// (fc, Mar 13/2011) display temperature between quotation lines;
				if (isTemperatureEnabled)
				{
					fetchTemperature();  // fills the text buffer with the temperature;
					return true;
				}
				break;

#ifdef WANT_TIME_AT_CITY
			case XTRA_CITY:
				// (mcm, Apr 30/2012) display city time between quotation lines;
				if ((isCityEnabled) && (city[cityIndex].name[0] != 0x0))
				{
					fetchCity();
					return true;
				}
				break;
#endif
		}
	} while(i != currentMessageType);
	return false;
}


//*********************************************************************************************************
//
int16_t WiseClock::runappStats()
{
	byte bMonth, bDay, bHour, bMinute, bSecond;
	int h, l, bYear, hTemp1, hTemp2, hYear, hMonth, hDay, lTemp1, lTemp2, lYear, lMonth, lDay; 
	char s = 'C';

	if (!isLineInProgress())
	{
		if (++setUpRepeat > 3)						// only show the stats a few times, cannot read the eeprom forever;
		{
  #ifdef _APP_ANIMATION_H_
			if (appAnimation.appAnimationState > 1)
				closeLongFile();
  #endif
			crtPos	=	9999;
			startUpAppQuote = false;
			restoreCrtApp();
			clearDisplay();
			return(100);
		}


#ifdef _WiFly_H_
		wiFly.getNet();
#endif
		crtColor = GREEN;
		if (largeTextFont == false)
			displayTime(8, false);
		memset(crtBuffer, ' ', BLANK_PREFIX);
		crtBuffer[BLANK_PREFIX] = 0;

		h = ((int)readUserSetting(allHighTemp1Loc) << 8) | ((int)readUserSetting(allHighTemp2Loc) & 0xFF);
		hYear = readUserSetting(allHighTempYearLoc) + 2000;
		hMonth = readUserSetting(allHighTempMonthLoc);
		hDay = readUserSetting(allHighTempDayLoc);

		l = ((int)readUserSetting(allLowTemp1Loc) << 8) | ((int)readUserSetting(allLowTemp2Loc) & 0xFF);
		lYear = readUserSetting(allLowTempYearLoc) + 2000;
		lMonth = readUserSetting(allLowTempMonthLoc);
		lDay = readUserSetting(allLowTempDayLoc);

		if (h > 240)
		{
			// probably old scaled-by-ten format, so re-scale and save
			h *= 2; h /= 5;		// 10x now 4x
			saveAllHighTemperature(h, hYear-2000, hMonth, hDay);
			
			l *= 2; l /= 5;		// 10x now 4x
			saveAllLowTemperature (l, lYear-2000, lMonth, lDay);
		}

		if (l < 240)		// 240 is 60 C scaled by 4 (140 F)
		{
			if (appTemperature.useCelsius == false)
			{
				l = (l * 9) / 5 + 128;
				h = (h * 9) / 5 + 128;
				s = 'F';
			}

			hTemp1 = h / 4;
			hTemp2 = ((h - (hTemp1 * 4)) * 10) / 4;
			lTemp1 = l / 4;
			lTemp2 = ((l - (lTemp1 * 4)) * 10) / 4;

			sprintf_P(&crtBuffer[5], PSTR("This year's high: %d.%d%c%c on %s %d. This year's low: %d.%d%c%c on %s %d. "),
				hTemp1, hTemp2, 0x7f, s, monthName[hMonth], hDay, lTemp1, lTemp2, 0x7f, s, monthName[lMonth], lDay);  // chr(0x7f) = degree symbol;
		}
			
		if (powerDownMonth < 13)
		{
			l = strlen(crtBuffer);
			sprintf_P(&crtBuffer[l], PSTR("  Power down at %02d:%02d  %s %d, %d "), powerDownHour, powerDownMinute, monthName[powerDownMonth], powerDownDay, powerDownYear);
		}

		getPowerUpTime(&bYear, &bMonth, &bDay, &bHour, &bMinute, &bSecond);
		l = strlen(crtBuffer);
		sprintf_P(&crtBuffer[l], PSTR("  Last (V%d.0) Boot at %02d:%02d:%02d %s %d, %d "),  _WISE_CLOCK_VER, bHour, bMinute, bSecond, monthName[bMonth], bDay, bYear);
#ifdef _WiFly_H_
		if (wiFly.mac[0] != 0) {
			l = strlen(crtBuffer);
			sprintf_P(&crtBuffer[l], PSTR("  WiFly MAC %s"), wiFly.mac);
		}
		if (wiFly.mac[0] != 0) {
			l = strlen(crtBuffer);
			sprintf_P(&crtBuffer[l], PSTR("  IP %s"), wiFly.addr);
		}
#endif
		resetCrtPos();
	}
	return(0);
}


//*********************************************************************************************************
void WiseClock::checkMenuActive()
{
	if (isMenuActive)
	{
		if (alarmClock.isTimeSetting)
		{
			displayTimeBlink(alarmClock.hour, alarmClock.minute, alarmClock.isAlarmEnabled);
		}
		else if (alarmClock.isAlarmSetting)
		{
			//*	<MLS> we want the minutes or seconds to blink while we are setting the alarm time
			displayTimeBlink(alarmClock.alarmHour, alarmClock.alarmMin, alarmClock.isAlarmEnabled);
		}
		else if (alarmClock.isDateSetting)
		{
			displayDateBlink();
		}
		else if (alarmClock.isDaySetting)
		{
			displayDayBlink();
		}
		else
		{
			displayTime(8, false);
		}

		// wait for the menu item to expire;
		// (fc, Feb 5/2011) consider all button pushes while the menu is being displayed;
		unsigned long ms = millis() - TIME_KEEP_MENU_ACTIVE;
		if (
		    ((long)(ms - timeBtnMenu) < 0)
		||  ((long)(ms - timeBtnSet ) < 0)
		||  ((long)(ms - timeBtnPlus) < 0)
		   )
		{
			// menu still active until it times out;
			return;
		}

		// menu should no longer be active, since time is up;
		isMenuActive	=	false;

		if (alarmClock.isAlarmSetting)
		{
			saveAlarmTime();

			alarmClock.isAlarmSoundOn		=	false;
			alarmClock.isAlarmTriggerEnabled	=	true;
			alarmClock.isAlarmSetting		=	false;
		}
		else if (alarmClock.isDateSetting || alarmClock.isDaySetting)
		{
			alarmClock.isDateSetting	=	false;
			alarmClock.isDaySetting		=	false;
			alarmClock.setDateOnly();
			reminderMsg[0] = 0;					// after date change no reminder set
		}
		else if (alarmClock.isTimeSetting )
		{
			// time is saved in RTC every time the Plus button is pressed;
			alarmClock.isTimeSetting	=	false;
		}

		item	=	-1;		//*	when we time out, set back to the beginning

		if (crtApp == APP_QUOTE)
		{
#ifdef _APP_QUOTE_H_
			// refresh the current time (after maybe an alarm set);
			displayTime(8, false);
#endif
#ifdef _APP_PONG_H_
		} else if (crtApp == APP_PONG)
		{
			// re-init the app;
			appPong.init();
#endif
		} else {
			// clear display for any other app;
			clearDisplay();
			crtPos = 9999;
		}
	}
}


//*********************************************************************************************************
void WiseClock::runCrtApp()
{
	int16_t ms = 100;

#ifdef _APP_SUN_H_
 #if NUM_DISPLAYS > 1
	appSun.run();
 #endif
#endif
	if (isMenuActive)
	{
		nextRun = millis() + ms;
		return;
	}

	if ((DSTPlus) && (!DSTactive) && (alarmClock.hour == 2))
	{
		DSTPlus = false;
		alarmClock.incDSTHour();		// Daylight Saving Time, increment the hour, 2 -> 3;
		DSTactive = true;
		saveDSTactive();
	}

	if ((DSTMinus) && (DSTactive) && (alarmClock.hour == 3))
	{
		DSTMinus = false;
		alarmClock.decDSTHour();		// Daylight Saving Time, decrement the hour, 3 -> 2;
		DSTactive = false;
		saveDSTactive();
	}

	if (!alarmClock.hour == 0)
	{	
		reminderTriggerEnabled = true;
	}	
	else
	{
		if (reminderTriggerEnabled == true)
		{
			if (alarmClock.minute >= 0)
			{				// It is Midnight, check for new reminder to be displayed;
				reminderTriggerEnabled = false;
				getMessageAndReminder(getMessageNumber());
#ifdef _APP_BIG_H_
				if (appBig.bigStartMode != readUserSetting(bigStartModeLoc))
					saveUserSetting(bigStartModeLoc, appBig.bigStartMode);
#endif
			}	
		}	
	}

#ifdef _WiFly_H_
	if ((alarmClock.minute == wiFly.tsMinute) && (alarmClock.second == wiFly.tsSecond))
		wiFly.timeSet();
#endif

	switch (crtApp)
	{

#ifndef SERIAL_MSG
		case APP_MSG:
			ms = runappMsg();
			break;
#endif
		case APP_NEWSD:
			ms = runappNewSD();
			break;

		case APP_STATS:
			ms = runappStats();
			break;
#ifdef WANT_EVENT_LOG
		case APP_LOG_CLEAR:
			ms = runappLogClear();
			break;
#endif

		case APP_SET_POWER_ON:
			ms = runappSetPowerOn();
			break;

		default:	// (fc, Sep 1, 2013) for all apps;
			ms = pCrtApp->run();

	}
			
	nextRun = millis() + ms;
}


//*********************************************************************************************************
// Read and write alarm status
boolean WiseClock::getAlarmOnOff()
{
	return boolean( readUserSetting(alarmOnOffLoc) );
}

//*********************************************************************************************************
void WiseClock::saveAlarmOnOff()
{
	saveUserSetting( alarmOnOffLoc, alarmClock.isAlarmEnabled); 
}


//*********************************************************************************************************
// Read and write alarm time (hour, minute);
void WiseClock::readAlarmTime()
{
	alarmClock.alarmMin	=	readUserSetting( alarmMinsLoc );
	alarmClock.alarmHour	=	readUserSetting( alarmHoursLoc );
}

//*********************************************************************************************************
void WiseClock::saveAlarmTime()
{
	saveUserSetting( alarmMinsLoc,	alarmClock.alarmMin );
	saveUserSetting( alarmHoursLoc,	alarmClock.alarmHour );
#ifdef _DEBUG_
	Serial.print("Alarm time saved to eeprom: ");
	Serial.print(alarmClock.alarmHour);
	Serial.print(":");
	Serial.println(alarmClock.alarmMin);
#endif
}


//*********************************************************************************************************
// Read and write LED brightness level;
int8_t WiseClock::getBrightness()
{
	return readUserSetting( brightnessLoc );
}

//*********************************************************************************************************
void WiseClock::saveBrightness()
{
	saveUserSetting( brightnessLoc, nBrightness); 
#ifdef _DEBUG_
	Serial.print("Saved brightness: ");
	Serial.println(nBrightness);
#endif
}


//*********************************************************************************************************
// Read and write snooze time
int8_t WiseClock::getSnoozeTime()
{
	return readUserSetting( snoozeTimeLoc ); 
}

//*********************************************************************************************************
void WiseClock::saveSnoozeTime()
{
	saveUserSetting( snoozeTimeLoc, snoozeMins ); 
#ifdef _DEBUG_
	Serial.print("Saved snooze time: ");
	Serial.println(snoozeMins);
#endif
}


//*********************************************************************************************************
// Read and write scrolling speed;
int8_t WiseClock::getSpeed()
{
	return readUserSetting( speedLoc ); 
}

//*********************************************************************************************************
void WiseClock::saveSpeed()
{
	saveUserSetting( speedLoc, nSpeed); 
#ifdef _DEBUG_
	Serial.print("Saved speed: ");
	Serial.println(nSpeed);
#endif
}


//*********************************************************************************************************
// Read and write param indicating if the date is displayed between quotes;
boolean WiseClock::getShowDate()
{
	return boolean( readUserSetting(showDateLoc) );
}

//*********************************************************************************************************
void WiseClock::saveShowDate()
{
	saveUserSetting( showDateLoc, isDateEnabled); 
#ifdef _DEBUG_
	Serial.print("Saved show date: ");
	Serial.println(isDateEnabled, HEX);
#endif
}


//*********************************************************************************************************
// Read and write param indicating if the Reminder message is displayed between quotes;
boolean WiseClock::getShowReminder()
{
	return boolean( readUserSetting(showReminderLoc) );
}

//*********************************************************************************************************
void WiseClock::saveShowReminder()
{
	saveUserSetting( showReminderLoc, isReminderEnabled); 
}

//*********************************************************************************************************
// Read and write param indicating if a special message is displayed between quotes;
boolean WiseClock::getShowMessage()
{
	return boolean( readUserSetting(showMessageLoc) );
}

//*********************************************************************************************************
void WiseClock::saveShowMessage()
{
	saveUserSetting( showMessageLoc, isMessageEnabled); 
#ifdef _DEBUG_
	Serial.print("Saved show message value: ");
	Serial.println(isMessageEnabled, HEX);
#endif
}

//*********************************************************************************************************
// Read and write param indicating if 24 hour mode is on;
boolean WiseClock::get24HourMode()
{
	return boolean( readUserSetting(mode24HourLoc) );
}

//*********************************************************************************************************
void WiseClock::save24HourMode()
{
	saveUserSetting( mode24HourLoc, alarmClock.is24hourModeEnabled); 
#ifdef _DEBUG_
	Serial.println("Saved 24 hour mode");
#endif
}
//*********************************************************************************************************
// Read and write param indicating if Chime is on;
boolean WiseClock::getChime()
{
	return boolean( readUserSetting(chimeLoc) );
}


//*********************************************************************************************************
void WiseClock::saveChime()
{
	saveUserSetting( chimeLoc, isChimeEnabled); 
#ifdef _DEBUG_
    Serial.println("Saved Chime mode");
#endif
}


#ifdef WANT_EVENT_LOG
//*********************************************************************************************************
// Read and write param indicating if logging is on;
boolean WiseClock::getLog()
{
	return boolean( readUserSetting(logLoc) );
}


//*********************************************************************************************************
void WiseClock::saveLog()
{
	saveUserSetting( logLoc, isLogEnabled);
#ifdef _DEBUG_
	Serial.println("Saved Logging mode");
#endif
}
#endif


//*********************************************************************************************************
// Read and write "show temperature" flag;
boolean WiseClock::getShowTemperature()
{
	return boolean (readUserSetting( showTemperatureLoc ));
}

//*********************************************************************************************************
void WiseClock::saveShowTemperature()
{
	saveUserSetting( showTemperatureLoc, isTemperatureEnabled); 
#ifdef _DEBUG_
	Serial.print("Saved show temperature: ");
	Serial.println(isTemperatureEnabled, HEX);
#endif
}

#ifdef WANT_TIME_AT_CITY
//*********************************************************************************************************
// Read and write "show city" flag;
boolean WiseClock::getShowCity()
{
	return boolean (readUserSetting( showCityLoc ));
}

//*********************************************************************************************************
void WiseClock::saveShowCity()
{
	saveUserSetting( showCityLoc, isCityEnabled); 
}
#endif


//*********************************************************************************************************
// Read and write "All Time High Temperature" ;
void WiseClock::getAllHighTemperature(int16_t* allHighTemp,  int8_t* allHighYear, int8_t* allHighMonth, int8_t* allHighDay)
{
	*allHighTemp = ((int)readUserSetting(allHighTemp1Loc) << 8) | ((int)readUserSetting(allHighTemp2Loc) & 0xFF);
	*allHighYear = readUserSetting(allHighTempYearLoc); 
	*allHighMonth = readUserSetting(allHighTempMonthLoc); 
	*allHighDay = readUserSetting(allHighTempDayLoc); 
}

//*********************************************************************************************************
void WiseClock::saveAllHighTemperature(int16_t high24Temp, int8_t year, int8_t month, int8_t day)
{
	saveUserSetting(allHighTemp1Loc, high24Temp >> 8);
	saveUserSetting(allHighTemp2Loc, high24Temp & 0xFF);
	saveUserSetting(allHighTempYearLoc, year);
	saveUserSetting(allHighTempMonthLoc, month);
	saveUserSetting(allHighTempDayLoc, day);
}



//*********************************************************************************************************
// Read and write "All Time Low Temperature" ;
void WiseClock::getAllLowTemperature(int16_t* allLowTemp, int8_t* allLowYear, int8_t* allLowMonth, int8_t* allLowDay)
{
	*allLowTemp = ((int)readUserSetting(allLowTemp1Loc) << 8) | ((int)readUserSetting(allLowTemp2Loc) & 0xFF);
	*allLowYear = readUserSetting(allLowTempYearLoc); 
	*allLowMonth = readUserSetting(allLowTempMonthLoc); 
	*allLowDay = readUserSetting(allLowTempDayLoc); 
}

//*********************************************************************************************************
void WiseClock::saveAllLowTemperature(int16_t low24Temp, int8_t year, int8_t month, int8_t day)
{
	saveUserSetting(allLowTemp1Loc, low24Temp >> 8);
	saveUserSetting(allLowTemp2Loc, low24Temp & 0xFF);
	saveUserSetting(allLowTempYearLoc, year);
	saveUserSetting(allLowTempMonthLoc, month);
	saveUserSetting(allLowTempDayLoc, day);
}


//*********************************************************************************************************
/*
void WiseClock::saveTempOffset(byte tempCorrection)
{
	saveUserSetting(tempOffsetLoc, tempCorrection);
}
*/


//*********************************************************************************************************
// Read and write "Personal Message Number (0-9)";
boolean WiseClock::getMessageNumber()
{
	return byte (readUserSetting(messageNumberLoc ));
}

//*********************************************************************************************************
void WiseClock::saveMessageNumber(byte messageNumber)
{
	saveUserSetting(messageNumberLoc, messageNumber); 
#ifdef _DEBUG_
	Serial.print("Personal Message Number: ");
	Serial.println(messageNumberLoc, HEX);
#endif
}


//*********************************************************************************************************
void WiseClock::saveLastPowerUpDate(byte day, byte month, byte year)
{	
	byte sum;
	saveUserSetting(lastCurrent1DayLoc, day);
	saveUserSetting(lastCurrent1MonthLoc, month);
	saveUserSetting(lastCurrent1YearLoc, year);
	sum = day + month + year;
	saveUserSetting(lastCurrent1SumLoc, sum);
	
	// save current time at 2nd location in case power goes down while saving the time
	saveUserSetting(lastCurrent2DayLoc, day);
	saveUserSetting(lastCurrent2MonthLoc, month);
	saveUserSetting(lastCurrent2YearLoc, year);
	saveUserSetting(lastCurrent2SumLoc, sum);
}


//*********************************************************************************************************
// Save date/time when power went down and came back;
void WiseClock::saveNewPowerUpDownTime()
{
	byte sum, year;
	powerDownMinute = RTC.getAl2Min();						// cannot save in EEPROM, more then 100,000 cycles;
	if (powerDownMinute > 59)							// if invalid value,
		powerDownMinute = 99;							// Show error
	powerDownHour = RTC.getAl2Hour();						// cannot save in EEPROM, more then 100,000 cycles;
	powerDownDay = readUserSetting(lastCurrent1DayLoc);
	powerDownMonth = readUserSetting(lastCurrent1MonthLoc);
	year = readUserSetting(lastCurrent1YearLoc);
	sum = readUserSetting(lastCurrent1SumLoc);
	if ((powerDownDay + powerDownMonth + year) != sum)
	{
		powerDownDay = readUserSetting(lastCurrent2DayLoc);
		powerDownMonth = readUserSetting(lastCurrent2MonthLoc);
		year = readUserSetting(lastCurrent2YearLoc);
	}

#ifdef WANT_EVENT_LOG
	updateLogFile("OF", powerDownHour, powerDownMinute, powerDownDay, powerDownMonth, year, 0,0);
	updateLogFile("ON", alarmClock.hour, alarmClock.minute, alarmClock.day, alarmClock.month, alarmClock.year - 2000, 0,0);
#endif
	powerDownYear = year + 2000;

	saveUserSetting(lastBootSecondLoc, alarmClock.second);
	saveUserSetting(lastBootMinuteLoc, alarmClock.minute);
	saveUserSetting(lastBootHourLoc,   alarmClock.hour);
	saveUserSetting(lastBootDayLoc,    alarmClock.day);
	saveUserSetting(lastBootMonthLoc,  alarmClock.month);
	saveUserSetting(lastBootYearLoc,   alarmClock.year - 2000);
}


//*********************************************************************************************************
// Get date and time when power was applied to the clock;
void WiseClock::getPowerUpTime(int* bYear, byte* bMonth, byte* bDay, byte* bHour, byte* bMinute, byte* bSecond)
{
	*bYear = readUserSetting(lastBootYearLoc) + 2000;
	*bMonth = readUserSetting(lastBootMonthLoc);
	*bDay = readUserSetting(lastBootDayLoc);
	*bHour = readUserSetting(lastBootHourLoc);
	*bMinute = readUserSetting(lastBootMinuteLoc);
	*bSecond = readUserSetting(lastBootSecondLoc);
}

//*********************************************************************************************************
// Read and write Font Size;
boolean WiseClock::getFontSize()
{
	return boolean (readUserSetting(fontSizeLoc));
}

//*********************************************************************************************************
void WiseClock::saveFontSize()
{
	saveUserSetting(fontSizeLoc, largeTextFont); 
}


//*********************************************************************************************************
// Read and write use Celsius Yes/No;
/*
boolean WiseClock::getUseCelsius()
{
	return boolean (readUserSetting(useCelsiusLoc));
}
*/


//*********************************************************************************************************
/*
void WiseClock::saveUseCelsius()
{
	saveUserSetting(useCelsiusLoc, useCelsius); 
}
*/

//*********************************************************************************************************
// Read and write DST active Yes/No;
boolean WiseClock::getDSTactive()
{
	return boolean (readUserSetting(DSTactiveLoc));
}

//*********************************************************************************************************
void WiseClock::saveDSTactive()
{
	saveUserSetting(DSTactiveLoc, DSTactive); 
}

//*********************************************************************************************************
// read stored data;
void WiseClock::initSavedParams()
{
	readAlarmTime();
	nSpeed				=	getSpeed();
	nBrightness			=	getBrightness();
	snoozeMins			=	getSnoozeTime();
	alarmClock.isAlarmEnabled	=	getAlarmOnOff();
	isDateEnabled			=	getShowDate();
	isMessageEnabled		=	getShowMessage();
#ifdef WANT_TIME_AT_CITY
	isCityEnabled			=	getShowCity();
#endif
	isTemperatureEnabled		=	getShowTemperature();
	isChimeEnabled	   		=	getChime();
#ifdef _APP_ANIMATION_H_
	appAnimation.isAnimEnabled	=	appAnimation.getAnim();
	appAnimation.appAnimationState	=	0;
#endif
#ifdef WANT_EVENT_LOG
	isLogEnabled			=	getLog();
#endif
	alarmClock.is24hourModeEnabled	=	get24HourMode();
	isReminderEnabled		=	getShowReminder();
	largeTextFont			= 	getFontSize();
	appTemperature.useCelsius	= 	appTemperature.getUseCelsius();
	// (fc, Jul 5/10)
#ifdef _APP_UTC_H_
	utcDiff				=	appUtc.getUtcDiff();
#else
	utcDiff				=	DEFAULT_UTC_DIFF;
#endif
	DSTactive			=	getDSTactive();
	
#ifdef _APP_BIG_H_
	appBig.bigStartMode		=	readUserSetting(bigStartModeLoc);
#endif
	// (fc, Feb 2/10) when powered for the first time, the values are not there (they come back as nonsense);
	// we need to initialize them with defaults and save them;
	if (alarmClock.alarmHour > 23 || alarmClock.alarmMin > 59 ||  nSpeed > MAX_SPEED || nBrightness > MAX_BRIGHTNESS || snoozeMins > MAX_SNOOZE_MINUTES)
	{
		// at least one param came as out-of-bounds, probably because it was never written to memory;
		// set all params with default values and save them;
		nSpeed			=	DEFAULT_SPEED;
		saveSpeed();

		nBrightness		=	DEFAULT_BRIGHTNESS;
		saveBrightness();

		snoozeMins		=	DEFAULT_SNOOZE_MINUTES;
		saveSnoozeTime();

		alarmClock.alarmHour	=	DEFAULT_ALARM_HOUR;
		alarmClock.alarmMin	=	DEFAULT_ALARM_MIN;
		saveAlarmTime();

		alarmClock.isAlarmEnabled	=	false;
		saveAlarmOnOff();

		isDateEnabled		=	false;
		saveShowDate();
		
		isTemperatureEnabled	=	false;
		saveShowTemperature();

		saveAllHighTemperature(-800, 0, 1, 1);
		saveAllLowTemperature (+800, 0, 1, 1);
		appTemperature.saveTempOffset( -3 );

		isMessageEnabled	=	false;
		saveShowMessage();
		saveMessageNumber(1);
		
		isReminderEnabled	=	false;
		saveShowReminder();

#ifdef WANT_TIME_AT_CITY
		isCityEnabled		=	false;
		saveShowCity();
#endif

		isChimeEnabled     	=	false;
		saveChime();

#ifdef _APP_UTC_H_
		utcDiff			=	DEFAULT_UTC_DIFF;
		appUtc.saveUtcDiff();
#endif

		largeTextFont		=	DEFAULT_LARGE_TEXT_FONT;
		saveFontSize();

		appTemperature.useCelsius	=	DEFAULT_USE_CELSIUS;
		appTemperature.saveUseCelsius();

#ifdef _APP_QUOTE_H_
		appQuote.saveQuoteFileDigit(1);			// start with quot1.txt file;
#endif
#ifdef _APP_WORDS_H_
		appWords.saveWordsFileDigit(1);		// start with word1.txt file;
#endif
#ifdef	_APP_ANIMATION_H_
		appAnimation.saveAnimFileDigit(1);	// start with anim1.csv file;
#endif

#ifdef _APP_TIMECLOCK_H_
		appTimeClock.tcProject = 1;
		appTimeClock.saveCurrentTCProject();

		for (byte i = 1; i < 6; i++) 
			appTimeClock.saveTimeClockdetails( i, 0, 0, 0);			// Init project 1 - 5;
#endif
		DSTactive = false;
		saveDSTactive();
	}


#ifdef _DEBUG_
	Serial.println("Params restored from eeprom: ");
	Serial.print("Alarm time: ");
	Serial.print(alarmClock.alarmHour);
	Serial.print(":");
	Serial.println(alarmClock.alarmMin);
	Serial.print("Speed: ");
	Serial.println(nSpeed);
	Serial.print("Brightness: ");
	Serial.println(nBrightness);
	Serial.print("Is alarm enabled: ");
	Serial.println(alarmClock.isAlarmEnabled, HEX);
	Serial.print("Is date enabled: ");
	Serial.println(isDateEnabled, HEX);
	Serial.print("Is message enabled: ");
	Serial.println(isMessageEnabled, HEX);
	Serial.print("Is temperature enabled: ");
	Serial.println(isTemperatureEnabled, HEX);
	Serial.print("UTC time difference: ");
	Serial.println(utcDiff);
#endif
}

//*********************************************************************************************************
// since time may have changed, find out what colour will be displayed with;
// depending on how close the time is to the alarm time, color could be:
// red    - one hour and less;
// yellow - three hours to one hour
// green  - more than three hours;
// the colors are applied only when alarm is enabled;
//
byte WiseClock::getColor()
{	
	byte color = GREEN;
	if (alarmClock.isAlarmEnabled)
        {
		int time = alarmClock.hour * 60  + alarmClock.minute;
		int alarmTime = alarmClock.alarmHour * 60 + alarmClock.alarmMin;

          // once passed the alarm time, look at the next one;
		if (time > alarmTime) 
			alarmTime += 1440;  // 24*60
		if (alarmTime - time > 180)  // 3*60
		{
			color = GREEN;
		}
		else if (alarmTime - time > 60)
		{
			color = ORANGE;
		}
		else //if (alarmTime - time > 0)
		{
			color = RED;
		}
	}
	return color;	
}

//*********************************************************************************************************
// clear a character-line of the display
void WiseClock::clearLine(int8_t ypos)
{
	memset(tempBuffer, ' ', BLANK_PREFIX);
	tempBuffer[BLANK_PREFIX] = 0;
	displayStaticLine(tempBuffer, ypos, BLACK);
}

//*********************************************************************************************************
void	WiseClock::displayTime(int8_t hpos, boolean secondsRequired)
{
	int8_t tmp;
	byte color = getColor();


	tmp = alarmClock.get12Hour();
	
	// build the string containing the formatted time;
	tempBuffer[0]	=	(tmp < 10) ? ' ' : ('0' + tmp/10);
	tempBuffer[1]	=	'0' + tmp % 10;

	tmp = alarmClock.second;
	if ((tmp & 1) == 0)
	{
		tempBuffer[2] = ':';
		tempBuffer[5] = ':';
	}
	else
	{
		if (alarmClock.isAlarmEnabled)
		{
			tempBuffer[2] = '.';
			tempBuffer[5] = '.';
		}
		else
		{
			tempBuffer[2] =  ' ';
			tempBuffer[5] =  ' ';
		}
	}
	tempBuffer[6]	=	'0' + tmp / 10;
	tempBuffer[7]	=	'0' + tmp % 10;

	tmp = alarmClock.minute;
	tempBuffer[3]	=	'0' + tmp / 10;
	tempBuffer[4]	=	'0' + tmp % 10;

#ifdef MULTI_DISPLAY_SCROLL
	tempBuffer[8]	=	0;
	displayStaticLine(tempBuffer, hpos, color);
#else
	if (!secondsRequired)
	{	
		tempBuffer[5] = 0;
		displayStaticLine(tempBuffer, hpos, color);
		return;
	}	
	// display time with seconds on hpos;
	for (int8_t i=0; i<8; i++)
	{
		ht1632_putTinyChar(i*4-1, hpos, tempBuffer[i], color);
	}
#endif
}

//*********************************************************************************************************
void	WiseClock::displayTimeBlink(int8_t hour, int8_t minute, boolean isAlarmEnabled)
{
	// build the string containing the formatted time;
	tempBuffer[0]	=	(hour < 10) ? ' ' : ('0' + hour/10);
	tempBuffer[1]	=	'0' + hour % 10;
	tempBuffer[2]	=	isAlarmEnabled? '.' : ':';
	tempBuffer[3]	=	'0' + minute / 10;
	tempBuffer[4]	=	'0' + minute % 10;

	//*	blink the digits that we are setting;
	if (((millis() / 250)  % 2) == 0)
	{
		if (isSettingHours)
		{
			tempBuffer[0]	=	0x20;
			tempBuffer[1]	=	0x20;
		}
		else
		{
			tempBuffer[3]	=	0x20;
			tempBuffer[4]	=	0x20;
		}
	}

	// display time on the bottom half of the display;
	for (int i=0; i<5; i++)
	{
		// (fc, Jan 29/2011) use 6-column digits since there is enough room on the 3216 display;
#ifdef MULTI_DISPLAY_SCROLL
		ht1632_putchar((X_MAX-32)/2 + 2 + i*6-1, 8, tempBuffer[i], GREEN);
#else
		ht1632_putchar(2 + i*6-1, 8, tempBuffer[i], GREEN);
#endif
	}
}


//*********************************************************************************************************
// (fc, Feb 4/2011)
void	WiseClock::displayDateBlink()
{
	// build the string containing the date formatted as YYMMDD;
	int shortYear = alarmClock.year - 2000;
	tempBuffer[0]	=	'0' + shortYear/10;
	tempBuffer[1]	=	'0' + shortYear % 10;
	tempBuffer[2]	=	'0' + alarmClock.month/10;
	tempBuffer[3]	=	'0' + alarmClock.month % 10;
	tempBuffer[4]	=	'0' + alarmClock.day/10;
	tempBuffer[5]	=	'0' + alarmClock.day % 10;

	//*	blink the digits that we are setting;
	if (((millis() / 250)  % 2) == 0)
	{
		if (isSettingYear)
		{
			tempBuffer[0]	=	0x20;
			tempBuffer[1]	=	0x20;
		}
		else if (isSettingMonth)
		{
			tempBuffer[2]	=	0x20;
			tempBuffer[3]	=	0x20;
		}
		else if (isSettingDay)
		{
			tempBuffer[4]	=	0x20;
			tempBuffer[5]	=	0x20;
		}
	}

	// display the date on the bottom half of the display;
	for (int i=0; i<=5; i++)
	{
		// (fc, Jan 29/2011) use small font (5-column) to fit 6 digits;
#ifdef MULTI_DISPLAY_SCROLL
		ht1632_putchar((X_MAX-32)/2 + i*5, 8, tempBuffer[i], ORANGE|SMALL_CHAR);
#else
		ht1632_putchar(i*5, 8, tempBuffer[i], ORANGE|SMALL_CHAR);
#endif
	}
}


//*********************************************************************************************************
// (fc, Feb 4/2011)
void	WiseClock::displayDayBlink()
{
	// copy the name of the day; blanks before and after to clear the screen around the 3-letter day;
	tempBuffer[0] = ' ';
	strcpy(tempBuffer+1, dayName[alarmClock.dow]);

	//*	blink the day that we are setting;
	if (((millis() / 250)  % 2) == 0)
	{
		memset(tempBuffer+1, ' ', 3);
	}

	// display the day on the bottom half of the display;
	for (int i=0; i<=6; i++)
#ifdef MULTI_DISPLAY_SCROLL
		ht1632_putchar((X_MAX-32)/2 + i*6, 8, tempBuffer[i], ORANGE);
#else
		ht1632_putchar(i*6, 8, tempBuffer[i], ORANGE);
#endif
}

//*********************************************************************************************************
// Save the current crtApp and pointer;
//
void WiseClock::saveCrtApp()
{
	crtApp_bak  = crtApp;
	pCrtApp_bak = pCrtApp;
}

//*********************************************************************************************************
// Restore the current crtApp and pointer;
//
void WiseClock::restoreCrtApp()
{
	crtApp  = crtApp_bak;
	pCrtApp = pCrtApp_bak;
}


//*********************************************************************************************************
// Read from /write to internal EEPROM;
//
void WiseClock::saveUserSetting(int loc, int value)
{
	// write to internal eeprom;
	EEPROM.write(loc, value);
	delay(5);

}


//*********************************************************************************************************
byte WiseClock::readUserSetting(int loc)
{
	return EEPROM.read(loc);
}


//************************************************************************
/*
extern unsigned int __bss_end;
extern void *__brkval;

int	WiseClock::GetFreeMemory()
{
int free_memory;

	if((int)__brkval == 0)
	{
		free_memory = ((int)&free_memory) - ((int)&__bss_end);
	}
	else
	{
		free_memory = ((int)&free_memory) - ((int)__brkval);
	}
	return free_memory;
}
*/


WiseClock wiseClock;
