/*
 *********************************************************************************************************
 * Wise Clock sketch for Duino644 board + 3216 LED display from Sure Electronics;
 *           - displays text retrieved from SD card (file quotes.txt) and time from RTC (DS3231);
 *
 * Jan/11 by FlorinC (http://timewitharduino.blogspot.com/)
 *   Copyrighted and distributed under the terms of the Berkeley license
 *   (copy freely, but include this notice of original author.)
 *
 * Parts copied and/or adapted from:
 *   - HT1632 library by Bill Westfield ("WestfW") (http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1225239439/0);
 *   - SDuFAT by David Cuartielles (http://www.arduino.cc/playground/Learning/SDMMC);
 *   - fWrite (fast writes) functions courtesy of BroHogan (http://brohogan.blogspot.com/);
 *   - Tone library by B.Hagman (http://code.google.com/p/arduino-tone/);
 *   - DS1307 class from the Arduino forum (http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1191209057)
 *        (download library from http://code.google.com/p/libds1307/);
 *
 *********************************************************************************************************
 *
 *********************************************************************************************************
 * TODO:
 *	1. (DONE) set date (day/month/year) from buttons;
 *	2. (DONE) zero second for accurate time setting;
 *	3. (DONE) display time with different colors, indicating the time left before the alarm goes on (as in IllyClock);
 *	4. (DONE) dimming the display from button "Set";
 *	5. (DONE) "big clock" mode: display seconds (use SQW interrupt from DS3231);
 *	6. (DONE) chime for hour and half hour (enable/disable);
 *
 *********************************************************************************************************
 */

//*********************************************************************************************************
//*	Edit History, started April, 2010
//*	please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//* Jan 28, 2011	<fc>  adapted from Wise4Sure.pde (sketch for Wise Clock 2);
//* Oct 15, 2011	<rp>  added saveNewPowerUpDownTime();
//* May 28, 2012	<mcm> scroll, sound, and apps are now asynchronous, based on elapsed time
//*
//*********************************************************************************************************


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif
#include <Wire.h>
#include "DS3231.h"
#include "HT1632.h"
#include "Sound.h"
#include "Buttons.h"
#include "AlarmClock.h"
#include "WiseClock.h"

//#define _DEBUG_ true


//*********************************************************************************************************
void setup()
{
#ifdef _DEBUG_
	Serial.begin(9600);
	Serial.println("in setup: ");
#endif

	setupSpeaker();

	setupButtons();

	beep();

	// read parameter values (speed, alarm, brightness etc) as saved in EEPROM;
	wiseClock.initSavedParams();

	alarmClock.getTimeFromRTC();
	alarmClock.initPrevious();

	ht1632_setup();

	wiseClock.initSerial();	/* Must be before initMessageFromSDcard() */

	wiseClock.initSdCard();

	wiseClock.initMessageFromSDcard();

	wiseClock.saveNewPowerUpDownTime();

	wiseClock.initCrtBuf();

	wiseClock.initApp();	/* Must be last */
}


//*********************************************************************************************************
void loop()
{
	unsigned long ms = millis();

	checkSound(ms);
	wiseClock.checkSerial();
	wiseClock.checkScroll(ms);

	if ((long)(ms - wiseClock.nextRun) >= 0)
	{
		alarmClock.getTimeFromRTC();
		alarmClock.checkAlarm();
		wiseClock.checkMenuActive();
		wiseClock.runCrtApp();
	}
}
