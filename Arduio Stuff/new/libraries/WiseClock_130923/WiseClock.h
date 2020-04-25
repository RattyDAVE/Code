/*
 *********************************************************************************************************
 * WiseClock.h
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
//* Apr 15/10 (fc) created class, by restructuring Wise4Sure.pde;
//* Oct 10/10 (rp) added changes for multiple big fonts
//* Jan 30/11 (fc) added "show temperature" feature;
//* Jun 20/11 (rp) added "chime, reminder" feature;
//* Oct 15/11 (rp) added Stopwatch, countdown, Words, Tix, Stats, Time clock
//* Apr 07/12 (mcm) quotation lines now have unlimited length
//* Apr 12/12 (mcm) add Dst flag
//* Apr 19/12 (mcm) Demo now cycles through display modes
//* Apr 27/12 (mcm) move quote, Utc apps to their own file
//* Apr 27/12 (mcm) rename some quote variables to use "crt" prefix
//* Apr 27/12 (mcm) remove quoteBufferPtr, add crtLen and resetCrtPos()
//* May 01/12 (mcm) option to display time at various cities (#define WANT_TIME_AT_CITY)
//* May 01/12 (mcm) option to display message from serial1 port (Bluetooth?) (#define SERIAL_MSG)
//* May 02/12 (mcm) use #define for scroll prefix
//* May 05/12 (mcm) use int8_t or int16_t instead of int where appropriate
//* Sep 01/13 (fc) moved app-specific code into their own app classes
//* Sep 10/13 (mcm) moved common config options to UserConf.h
//*********************************************************************************************************


#ifndef _WISE_CLOCK_
#define _WISE_CLOCK_

#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "UserConf.h"
#include "AppBase.h"


// names of the possible "applications";
enum {
	APP_QUOTE, APP_UTC, APP_BIG, APP_LIFE, APP_DEMO, APP_PONG, APP_PACMAN, APP_LIVED,
	APP_SCORE, APP_STOPW, APP_CNT_DOWN, APP_WORDS, APP_MSG, APP_STATS, APP_TCLOK,
	APP_TIX, APP_LINES, APP_SUN, APP_ANIM, APP_LOG_CLEAR, APP_TMP, APP_NEWSD,
	APP_SET_POWER_ON
};


// maximum length of a reminder/personel messsage;
#define MAX_SMSG_LEN	200

// how much to read from the quote file at a time
// must be at least 150, and at least 10 more than MAX_SMSG_LEN
// quote lines themselves are unlimited, this sets how much to process in one go
#define MAX_MSG_LEN	220

// maximum length of a city name
#define MAX_CITY_LEN	16

// alarm stops beeping after these many minutes;
#define DEFAULT_SNOOZE_MINUTES	2
#define MAX_SNOOZE_MINUTES	5


// default brightness is 3;
#define DEFAULT_BRIGHTNESS	3
#define MAX_BRIGHTNESS		5


// sets the scrolling speed;
// value between 0 (slowest) and 5 (fastest); translates into delay from 53ms to 13ms;
// delay            =  53 - nSpeed * 8;
#define MAX_SPEED	5
#define DEFAULT_SPEED	3


// default time zone (for UTC);
#define DEFAULT_UTC_DIFF	-5

// start with small font
#define DEFAULT_LARGE_TEXT_FONT false

// used for updating counters in button functions;
#define SET_BUTTON	0x1
#define PLUS_BUTTON	0x2
#define MENU_BUTTON	0x4

#ifndef WANT_APP_ANIM
 #undef WANT_ANIMATIONS
#endif


//*********************************************************************************************************
// store settings in internal EEPROM at these locations;
//
#define	alarmHoursLoc			0		// alarm hours storage location
#define	alarmMinsLoc			1		// alarm minutes storage location
#define	alarmOnOffLoc			2		// alarmOn storage location
#define	alarmModeLoc			3		// alarmMode storage location
#define	brightnessLoc			4		// brightness level storage location
#define	clockAMPMLoc			5		// clock AMPM vs 24hr storage location
#define	speedLoc			6		// speed storage location
#define	snoozeTimeLoc			7		// snooze time storage location
#define	showDateLoc			8		// "show date" storage location
#define	showMessageLoc			9		// "show message" storage location
#define	mode24HourLoc			10		// 12/24 hour mode
#define	utcDiffLoc			11		// utc difference (in hours);
#define	showTemperatureLoc 		12		// storage location for the "show temperature" flag;
#define	chimeLoc			13		// storage location for the "chime enabled" flag;
#define	showReminderLoc			14		// storage location for the "show Reminder message" flag;
#define	messageNumberLoc		15		// Current Personal Message number ( 0 - 9);

#define allHighTemp1Loc			16		// Low  byte of Highest Cel. Temperature;
#define allHighTemp2Loc			17		// High byte of Highest Cel. Temperature;
#define allHighTempDayLoc		18		// Day with highest Cel. Temperature;
#define allHighTempMonthLoc		19		// Month with highest Cel. Temperature;
#define allHighTempYearLoc		20		// Year - 2000 with highest Cel. Temperature;
#define allLowTemp1Loc			21		// Low  byte of Lowest Cel. Temperature;
#define allLowTemp2Loc			22		// High byte of Lowest Cel. Temperature;
#define allLowTempDayLoc		23		// Day with lowest Cel. Temperature;
#define allLowTempMonthLoc		24		// Month with lowest Cel. Temperature;
#define allLowTempYearLoc		25		// Year - 2000 with lowest Cel. Temperature;
	
#define	lastBootDayLoc			26		// Day that Clock started, set at startup;
#define	lastBootMonthLoc		27		// Month that Clock started;
#define	lastBootYearLoc			28		// Year that Clock started;
#define	lastBootHourLoc			29		// Hour that Clock started;
#define	lastBootMinuteLoc		30		// Minute that Clock started;
#define	lastBootSecondLoc		31		// Second that Clock started;

#define lastCurrent1SumLoc		32		// checksum of Last day, month, year - used to report start of power loss;
#define lastCurrent1DayLoc		33		// Last current Day1, used to report start of power loss;
#define lastCurrent1MonthLoc		34		// Last current Month1, used to report start of power loss;
#define lastCurrent1YearLoc		35		// Last current Year1 - 2000, used to report start of power loss;
#define lastCurrent2SumLoc		36		// checksum of Last day, month, year - used to report start of power loss;
#define lastCurrent2DayLoc		37		// Last current Day2, used to report start of power loss;
#define lastCurrent2MonthLoc		38		// Last current Month2, used to report start of power loss;
#define lastCurrent2YearLoc		39		// Last current Year2 - 2000, used to report start of power loss;

#define tClokCurrentProjectLoc		40		// Projectnumber 1 - 5;
// Project 1
#define tClok1StateLoc			41		// State 0 = no recording yet, 1 = recording started, 2 = recording ended;
#define tClok1StartHiLoc		42		// Start Day number * 24 * 60 + Hour * 60 byte high;
#define tClok1StartLoLoc		43		// Start Day number * 24 * 60 + Hour * 60 byte low;
#define tClok1TotalHoursB2Loc		44		// Total time of Project byte 2;
#define tClok1TotalHoursB1Loc		45		// Total time of Project byte 1;
// Project 2
#define tClok2StateLoc			46		// State 0 = no recording yet, 1 = recording started, 2 = recording ended;
#define tClok2StartHiLoc		47		// Start Day number * 24 * 60 + Hour * 60 byte high;
#define tClok2StartLoLoc		48		// Start Day number * 24 * 60 + Hour * 60 byte low;
#define tClok2TotalHoursB2Loc		49		// Total time of Project byte 2;
#define tClok2TotalHoursB1Loc		50		// Total time of Project byte 1;
// Project 3
#define tClok3StateLoc			51		// State 0 = no recording yet, 1 = recording started, 2 = recording ended;
#define tClok3StartHiLoc		52		// Start Day number * 24 * 60 + Hour * 60 byte high;
#define tClok3StartLoLoc		53		// Start Day number * 24 * 60 + Hour * 60 byte low;
#define tClok3TotalHoursB2Loc		54		// Total time of Project byte 2;
#define tClok3TotalHoursB1Loc		55		// Total time of Project byte 1;
// Project 4
#define tClok4StateLoc			56		// State 0 = no recording yet, 1 = recording started, 2 = recording ended;
#define tClok4StartHiLoc		57		// Start Day number * 24 * 60 + Hour * 60 byte high;
#define tClok4StartLoLoc		58		// Start Day number * 24 * 60 + Hour * 60 byte low;
#define tClok4TotalHoursB2Loc		59		// Total time of Project byte 2;
#define tClok4TotalHoursB1Loc		60		// Total time of Project byte 1;
// Project 5
#define tClok5StateLoc			61		// State 0 = no recording yet, 1 = recording started, 2 = recording ended;
#define tClok5StartHiLoc		62		// Start Day number * 24 * 60 + Hour * 60 byte high;
#define tClok5StartLoLoc		63		// Start Day number * 24 * 60 + Hour * 60 byte low;
#define tClok5TotalHoursB2Loc		64		// Total time of Project byte 2;
#define tClok5TotalHoursB1Loc		65		// Total time of Project byte 1;

#define fontSizeLoc			66		// Large or Normal Font;

#define useCelsiusLoc			67		// Use Celsius for temperature display;

#define quoteFileLoc			68		// Last digit of current in use quot?.txt file;
#define wordsFileLoc			69		// Last digit of current in use word?.txt file;
#define DSTactiveLoc			70		// Daylight savings time is in affect

#define animFileLoc			71		// Last digit of current in use anim?.bin file;
#define animLoc				72		// storage location for the "animation enabled" flag;

#define logLoc				73		// storage location for the "logging enabled" flag;

#define tempOffsetLoc			74		// storage location for the Temperature offset correction value;

#define showCityLoc			75		// storage location for the "show city time" flag;

#define powerOnAppLoc			76		// storage location for the app to run at power-on

#define bigStartModeLoc			77		// storage location for initial "Big" mode

// (fc, Sep 14, 2013) used by AppCountDownApp to record the target end time for countdown;
// the next 4 bytes store an unsigned long, in little endian;
#define countDownEndTimeLoc		78		// storage location for the end time of the countdown app;

// (fc, Sep 14, 2013) please make sure that the next eeprom address is 4 bytes bigger than the previous, since we just stored an unsigned long (4 bytes);
#define chronoModeLoc			82		// storage location for chrono mode; either conventional (C) or rattrapante (R);

// (fc, Sep 21, 2013) locations for AppScore;
#define score1Loc			83
#define score2Loc			84
// (mcm, Sep 23, 2013) location for quote random/sequential flag
#define quoteRandLoc			85		// true == randomize quotes



//*********************************************************************************************************
class WiseClock
{
public:
  CAppBase* pCrtApp;		// (fc, Sep 1, 2013) control apps through polymorphism;
  CAppBase* pCrtApp_bak;

  byte crtApp;
  byte crtApp_bak;

  unsigned long nextRun;

  int8_t nSpeed;
  int8_t nBrightness;
  int8_t snoozeMins;
  int8_t plusButtonCnt;			// general counter incremented in Plus Button function;
  int8_t setButtonCnt;			// general counter incremented in Set  Button function;
  int8_t menuButtonCnt;			// general counter incremented in Menu Button function;
  byte buttonsInUse;			// if corresponding bit set increment plusButtonCnt/setButtonCnt/menuButtonCnt;
  
  boolean isSdCard;			// shows if the SD card is present (and can be initialized) or not;
  boolean isLongOpen;			// is a quote file open
  boolean isLongTrunc;			// is a truncated quote line
  boolean isLongComment;		// is a quote comment line
  boolean isShortOpen;			// is a message file open
  boolean isShortTrunc;			// is a truncated message line
  boolean isShortComment;		// is a message comment line

  // true when menu is on the screen;
  // determines the functionality of the Set and Plus buttons;
  boolean isMenuActive;

  // when the alarm time is set, either the hours or the minutes are active (incremented by Plus button);
  boolean isSettingHours;

  // used for setting up the date;
  boolean isSettingYear;
  boolean isSettingMonth;
  boolean isSettingDay;
  
  // Daylight Saving Time activated;
  boolean DSTPlus;
  boolean DSTMinus;
  int8_t utcDiff;			// offset from UTC, in hours

  // show (or not) date between quotes;
  boolean isDateEnabled;

  // show (or not) "personalized" message between quotes;
  boolean isMessageEnabled;
#ifdef SERIAL_MSG
  boolean checkMessageCmd;
#endif
  
  // show (or not) temperature between quotes;
  boolean isTemperatureEnabled;

#ifdef WANT_TIME_AT_CITY
  // show (or not) city time between quotes;
  boolean isCityEnabled;
#endif

  // sound (or not) the chime (sound/bells) for hour and half hour;
  boolean isChimeEnabled;

  // show (or not) reminder message;
  boolean isReminderEnabled;
  
  // log (or not)
  boolean isLogEnabled;

  boolean largeTextFont;		// use Large Font ?

  boolean DSTactive;			// currently in Daylight Savings Time
  
  int8_t setUpRepeat;
  byte powerDownMinute;			// the last Power down happened at this minute;
  byte powerDownHour;			// the last Power down happened at this hour;
  byte powerDownDay;			// the last Power down happened on this day;
  byte powerDownMonth;			// the last Power down happened in this month;
  int16_t  powerDownYear;		// the last Power down happened in this year BC;
  boolean startUpAppQuote;		// flag used during init App Quote;
  byte	showExtraMessages;		// state to alternate between quote message and other message like date, temp etc.
  // use this buffer to display the time (seconds are ignored);
  char	tempBuffer[14];			//	=	"12.45.03";
  char  longFile[14];			// cached long-duration file name (quote, animation)
  char  orgFile[14];			// previous long-duration file name
  // holds a personal message read from message.txt (on the SD card);
  char	personalMsg[MAX_SMSG_LEN + 2];

  char	crtBuffer[MAX_MSG_LEN + 2];	// allow for extra characters in case message too long;
  byte	crtColor;			// (fc, Jan 30/2011) display message with the crt color;
  int	crtPos;				// current display position in buffer
  unsigned long nextScroll;		// time of next scroll
  uint32_t longPos;			// seek position in current file
  
private:

  // holds the reminder message read from ??message.txt (on the SD card);
  char	reminderMsg[MAX_SMSG_LEN + 2];

  char  shortFile[14];			// cached short-duration file name (message, words)

#ifdef WANT_TIME_AT_CITY
  struct {
	char name[MAX_CITY_LEN+1];	// name of a city
	int8_t	utc;			// 2X scaled offset from UTC for city
	int16_t	dst_start;		// julian date in this year that DST starts
	int16_t	dst_end;		// julian date in this year that DST ends
	int16_t	lat;			// latitude of the city
	int16_t	lon;			// longitude of the city
  } city[10];
  int8_t cityIndex;			// what city to display
#endif

  boolean reminderTriggerEnabled;	// to prevent execution of the reminder function more than once;
  
  int8_t item;				// current item in the menu;
  uint8_t currentMessageType;		// current message type on the display;

  int	crtLen;				// length of string in buffer
  int8_t scrPos;			// scrolling position

#ifdef SERIAL_MSG
  int8_t pmPos;
#endif

  byte	appState;			// internal App state (LogClear, NewSD)

public:
  WiseClock();

  // (fc, Mar 26/2012) switched to SdFat (covers FAT32 as well);
  boolean	initSdCard();
  void	initSerial();

  void	checkMenuActive();
  void	checkCrtApp();
  void	runCrtApp();
  void  saveCrtApp();
  void  restoreCrtApp();
  
  void	processButtonPlus();
  void	processButtonMenu();
  void	processButtonSet();
  void	processSetDate();

  void	initSavedParams();
  void	initCrtBuf();
  void	resetCrtPos();
  void	initMessageFromSDcard();
  void  initApp();

  byte	getColor();
  void  clearLine(int8_t ypos);
  void	displayTime(int8_t hpos, boolean secondsRequired);
  void	displaySmallScrollingLine();
  void 	displayLargeScrollingLine();
  byte  colorSpec(int pos);

  boolean	fetchExtraInfo();
  void	fetchDate();
  void	fetchMessage();
  void	fetchTemperature();
  void	splitTemp(int temp, int* pITemp, int* pDTemp, boolean useC);
  void	fetchReminder();
  void  fetchCity();

  void	saveNewPowerUpDownTime();
  void 	saveLastPowerUpDate(byte day, byte month, byte year);

  boolean   openLongFile(const char* filename);
  void	    closeLongFile();
  boolean   readLineFromLongSD(char* buffer, int maxLength);
  int16_t   readFromLongSD(char* buf, int16_t len);
  uint32_t  tellLongFile();
  boolean   seekLongFile(uint32_t pos);
  boolean   openShortFile(const char* filename, uint8_t mode);
  void	    closeShortFile();
  boolean   readLineFromShortSD(char* buffer, int maxLength);
  boolean   getMessageAndReminder(byte pmNumber);
#ifdef WANT_EVENT_LOG
  byte      updateLogFile(char* type, int minute, int hour, int day, int month, int year, long p1, long p2);
  int16_t   runappLogClear();
#endif

  boolean   isLineInProgress();
  
  byte	getWordsFileDigit();
  void	saveWordsFileDigit(byte wordsFileDigit);
  boolean openWordsFile(byte wn);

  void 	getAllHighTemperature(int16_t* allHighTemp, int8_t* allHighYear, int8_t* allHighMonth, int8_t* allHighDay);
  void 	saveAllHighTemperature(int16_t high24Temp, int8_t year, int8_t month, int8_t day);
  void	getAllLowTemperature(int16_t* allLowTemp, int8_t* allLowYear, int8_t* allLowMonth, int8_t* allLowDay);
  void 	saveAllLowTemperature(int16_t low24Temp, int8_t year, int8_t month, int8_t day);

  boolean getDSTactive();
  void  saveDSTactive();
  void checkScroll(unsigned long ms);
  void checkSerial();

  void	saveUserSetting(int loc, int value);
  byte	readUserSetting(int loc);
  
private:
#ifndef SERIAL_MSG
  int16_t 	runappMsg();
#endif
  int16_t	runappStats();
  int16_t	runappNewSD();

  boolean	getAlarmOnOff();
  void	saveAlarmOnOff();
  void	readAlarmTime();
  void	saveAlarmTime();
  int8_t	getBrightness();
  void	saveBrightness();
  int8_t	getSnoozeTime();
  void	saveSnoozeTime();
  int8_t	getSpeed();
  void	saveSpeed();
  boolean	getShowDate();
  void	saveShowDate();
  boolean	getShowReminder();
  void	saveShowReminder();
  boolean	getShowMessage();
  void	saveShowMessage();
  boolean	getShowCity();
  void	saveShowCity();
  boolean	get24HourMode();
  void	save24HourMode();
  // (fc, Jan 30, 2011) get/set temperature flag;
  boolean	getShowTemperature();
  void	saveShowTemperature();

  // (fc, Feb 3, 2011) get/set chime flag;
  boolean	getChime();
  void	saveChime();
  byte	getMessageNumber();
  void	saveMessageNumber(byte messageNumber);
  boolean	getFontSize();
  void	saveFontSize();
  boolean	getLog();
  void  saveLog();

  void	getPowerUpTime(int* bYear, byte* bMonth, byte* bDay, byte* bHour, byte* bMinute, byte* bSecond);

  void	displayTimeBlink(int8_t hour, int8_t minute, boolean isAlarmEnabled);
  void	displayDateBlink();
  void	displayDayBlink();
  int16_t runappSetPowerOn();
  void  startApp(byte item, boolean setup);
};


extern WiseClock wiseClock;


#endif  // _WISE_CLOCK_

