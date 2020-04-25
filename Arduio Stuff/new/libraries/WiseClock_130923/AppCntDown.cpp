// AppCntDown.cpp

//*********************************************************************************************************
//*	Edit History, started Oct, 2011
//*	please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//*	Oct  15,	2011	(rp) first entry;
//*	May  28,	2012	(mcm) restructure to eliminate delay() calls
//*	Jun  25,	2012	(mcm) new args to displayTime()
//*
//*********************************************************************************************************

#include "UserConf.h"

#ifdef WANT_APP_CNT_DOWN

#include "AppCntDown.h"
#include "HT1632.h"
#include "WiseClock.h"
#include "AlarmClock.h"
#include "Sound.h"


void CAppCntDown::init(byte bFreshCounting/*=0*/)
{
  //---------------------------------------------------------------------------------------
  // (fc, Sep 14, 2013) if CountDown is the default app (config by the user in PRWON),
  //                    resume the count down even after power was off for a while;
  //---------------------------------------------------------------------------------------

/*
Serial.begin(9600);
Serial.println("in countdown app ");
*/

  clearDisplay();
  wiseClock.buttonsInUse = SET_BUTTON + PLUS_BUTTON;
  wiseClock.plusButtonCnt = 0;
  wiseClock.setButtonCnt = 0;
  blinkingDigit = 0;
  loopCnt = 0;

  if (bFreshCounting)
  {
	cntDownStartUp = true;
	cntDownTime = 0l;
  }
  else	// resume counting down to the original target time;
  {
	cntDownStartUp = false;
	freezeTime = false;

	startTime = (long)((alarmClock.hour * 3600l) + (alarmClock.minute * 60) + alarmClock.second);

	// get current time as unix time (seconds passed since Jan 1, 1970);
	time_t timeNow = makeUnixTime(alarmClock.year, alarmClock.month, alarmClock.day, alarmClock.hour, alarmClock.minute, alarmClock.second);

	// calculate the value of countdownTime based on the end time saved in eeprom;
	cntDownTime = getEndTime() - timeNow;

/*
Serial.print("Crt unix time is=");
Serial.println(timeNow);
Serial.print("Countdown seconds (endtime-now) =");
Serial.println(cntDownTime);
*/

	if (cntDownTime <= 0)
	{
		// countdown actually has stopped; go into countdown set up mode;
		cntDownStartUp = true;
		cntDownTime = 0l;
	}
  }
}


int16_t CAppCntDown::run()
{
	byte color;
	coord_t i;
	int16_t dly;
	long currentTime, newTime;

	dly = 64;

#ifndef MULTI_DISPLAY_SCROLL

	if (cntDownStartUp == true)
	{

		if (wiseClock.setButtonCnt > 0)					// if Set button pressed go to next digit;
		{
			wiseClock.setButtonCnt = 0;


			if (++blinkingDigit > 6)
				blinkingDigit = 0;
		}

		if (wiseClock.plusButtonCnt > 0)				// if Plus Button pressed increment digit;
		{
			wiseClock.plusButtonCnt = 0;

			newTime = cntDownTime;
			hour = (long)(newTime / 3600l);
			newTime = (long)(newTime % 3600l);
			minute = newTime / 60;
			second = newTime % 60;
			
			switch (blinkingDigit)					
			{
				case 0:
					if (hour > 19)
						cntDownTime = cntDownTime - 72000l;			// - 20 hours;
					else
						cntDownTime = (long)(cntDownTime + 36000l);		// + 10 hours;
					break;
				case 1:
					if (hour > 22)
						cntDownTime = cntDownTime - 10800l;			// - 3 hours; 
					else
						if (hour % 10 == 9)
							cntDownTime = cntDownTime - 32400l;		// - 9 hours;
						else
							cntDownTime = (long)(cntDownTime + 3600l);	// + 1 hour;
					break;
				case 2:
					if (minute > 49)
						cntDownTime = cntDownTime - 3000l;			// - 50 minutes;
					else
						cntDownTime = cntDownTime + 600l;			// + 10 minutes
					break;
				case 3:
					if (minute % 10 == 9)
						cntDownTime = cntDownTime - 540l;			// - 9 minutes
					else
						cntDownTime = cntDownTime + 60l;			// + 1 minutes;
					break;
				case 4:
					if (second > 49)
						cntDownTime = cntDownTime - 50l;			// - 50 seconds;
					else
						cntDownTime = cntDownTime + 10l;			// + 10 seconds;
					break;
				case 5:
					if (second % 10 == 9)
						cntDownTime = cntDownTime - 9l;				// - 9 seconds;
					else
						cntDownTime = cntDownTime + 1l;				// + 1 second;
					break;
				case 6:
					blinkingDigit = 999;							// we must start the count down;
			}
		}

		newTime = cntDownTime;
		hour = (long)(newTime / 3600l);
		newTime = (long)(newTime % 3600l);
		minute = newTime / 60;
		second = newTime % 60;
		
		// build the string containing the formatted time;
		wiseClock.tempBuffer[0]	=	'0' + hour / 10;
		wiseClock.tempBuffer[1]	=	'0' + hour % 10;
		wiseClock.tempBuffer[2]	=	':';
		wiseClock.tempBuffer[3]	=	'0' + minute / 10;
		wiseClock.tempBuffer[4]	=	'0' + minute % 10;
		wiseClock.tempBuffer[5]	=	':';
		wiseClock.tempBuffer[6]	=	'0' + second / 10;
		wiseClock.tempBuffer[7]	=	'0' + second % 10;
	
		//*	blink the digits that we are setting;
		if (millis() & 0x100)							// changes every 256 milliseconds
		{
			if (blinkingDigit < 6)
			{
				i = blinkingDigit;						// skip colon : seperator;
				if (i > 1)
					++i;
				if (i > 4)
					++i;
				wiseClock.tempBuffer[i]	=	' ';
				ht1632_putchar(10, 8, 'G', ORANGE);
				ht1632_putchar(16, 8, 'o', ORANGE);
			}
			else
			{	
				ht1632_putchar(10, 8, ' ', BLACK);
				ht1632_putchar(16, 8, ' ', BLACK);
			}	
		}
		else
		{
			ht1632_putchar(10, 8, 'G', ORANGE);
			ht1632_putchar(16, 8, 'o', ORANGE);
		}

		for (i=0; i<8; i++)
		{
			ht1632_putTinyChar(i*4-1, 1, wiseClock.tempBuffer[i], GREEN);
		}
			
		if (blinkingDigit == 999)					// Plus key pressed while "Go" was blinking will start the count down;
		{
			startTime = (long)((alarmClock.hour * 3600l) + (alarmClock.minute * 60) + alarmClock.second);
			wiseClock.buttonsInUse = SET_BUTTON;	// the Plus key can now be used to change the brightness;
			freezeTime = false;
			cntDownStartUp = false;

			// (fc, Sep 14, 2013) save the end time in eeprom, to be able to resume countdown later;
			time_t timeNow = makeUnixTime(alarmClock.year, alarmClock.month, alarmClock.day, alarmClock.hour, alarmClock.minute, alarmClock.second);
			saveEndTime(timeNow + cntDownTime);
/*
Serial.print("Save Crt unix time to eprom =");
Serial.println(timeNow);
*/
		}	
	}
	else
	{
		if (freezeTime == false)
		{
			currentTime = (long)((alarmClock.hour * 3600l) + (alarmClock.minute * 60) + alarmClock.second);
			if (currentTime < startTime)
				currentTime = currentTime + long(24l * 3600l);	// correction if we go from 23:59 - > 00:00;
			newTime = cntDownTime - (currentTime - startTime);
			if (newTime	< 0l) 					// in case we missed one or more second increments;
				newTime = 0;
			hour = (long)(newTime / 3600l);
			newTime = (long)(newTime % 3600l);
			minute = newTime / 60;
			second = newTime % 60;
		}
			
		// build the string containing the formatted time;
		wiseClock.tempBuffer[0]	=	'0' + hour / 10;
		wiseClock.tempBuffer[1]	=	'0' + hour % 10;
		wiseClock.tempBuffer[2]	=	':';
		wiseClock.tempBuffer[3]	=	'0' + minute / 10;
		wiseClock.tempBuffer[4]	=	'0' + minute % 10;
		wiseClock.tempBuffer[5]	=	':';
		wiseClock.tempBuffer[6]	=	'0' + second / 10;
		wiseClock.tempBuffer[7]	=	'0' + second % 10;

		color = ORANGE;
		if ((hour == 0) && (minute == 0))
			color = RED;
		
		for (i=0; i<8; i++)
		{
			ht1632_putTinyChar(i*4-1, 1, wiseClock.tempBuffer[i], color);
		}
		
		if (wiseClock.setButtonCnt > 0)
		{
			wiseClock.setButtonCnt = 0;
			freezeTime = !freezeTime;	
		}	

		if ((hour == 0) && (newTime == 0)) 					// ready
		{
			if (++loopCnt <= 6)
			{
				if (loopCnt & 1)
				{
					clearDisplay();
					beep();
				}
				else
				{
					ht1632_putchar(10, 8, 'G', RED);
					ht1632_putchar(16, 8, 'o', RED);
					for (byte i=0; i<8; i++)
						ht1632_putTinyChar(i*4-1, 1, wiseClock.tempBuffer[i], RED);
				}
				dly = 250;
			}
			else
			{
				init();								// Start all over again;
			}
		}
		else
		{
			wiseClock.displayTime(8, false);
		}
	}


//*****************************************************************************************************
#else	// for multi-display

	if (cntDownStartUp == true)
	{
		// setting up the timing (days, hours, minutes, seconds) for count down;

		if (wiseClock.setButtonCnt > 0)					// if Set button pressed go to next digit;
		{
			wiseClock.setButtonCnt = 0;
			if (++blinkingDigit > 8)
				blinkingDigit = 0;
		}

		if (wiseClock.plusButtonCnt > 0)				// if Plus Button pressed increment digit;
		{
			wiseClock.plusButtonCnt = 0;

			newTime = cntDownTime;
			day = (long)(newTime / 86400l);
			newTime = (long)(newTime % 86400l);	// reuse variable;
			hour = (long)(newTime / 3600l);
			newTime = (long)(newTime % 3600l);	// reuse variable again;
			minute = newTime / 60;
			second = newTime % 60;
			
			switch (blinkingDigit)					
			{
// set up the days;
				case 0:
					if (day > 99)
						cntDownTime = cntDownTime - 7776000l;			// - 90 days;
					else
						cntDownTime = (long)(cntDownTime + 864000l);		// + 10 days (86400 seconds/day);
					break;
				case 1:
					if (day > 9)
						cntDownTime = cntDownTime - 777600l;			// - 9 days; 
					else
						cntDownTime = (long)(cntDownTime + 86400l);		// + 1 day (86400 seconds/day);
					break;
// set up the hours;
				case 2:
					if (hour > 19)
						cntDownTime = cntDownTime - 72000l;			// - 20 hours;
					else
						cntDownTime = (long)(cntDownTime + 36000l);		// + 10 hours;
					break;
				case 3:
					if (hour > 22)
						cntDownTime = cntDownTime - 10800l;			// - 3 hours; 
					else
						if (hour % 10 == 9)
							cntDownTime = cntDownTime - 32400l;		// - 9 hours;
						else
							cntDownTime = (long)(cntDownTime + 3600l);	// + 1 hour;
					break;

// set up the minutes;
				case 4:
					if (minute > 49)
						cntDownTime = cntDownTime - 3000l;			// - 50 minutes;
					else
						cntDownTime = cntDownTime + 600l;			// + 10 minutes
					break;
				case 5:
					if (minute % 10 == 9)
						cntDownTime = cntDownTime - 540l;			// - 9 minutes
					else
						cntDownTime = cntDownTime + 60l;			// + 1 minutes;
					break;

// set up the seconds;
				case 6:
					if (second > 49)
						cntDownTime = cntDownTime - 50l;			// - 50 seconds;
					else
						cntDownTime = cntDownTime + 10l;			// + 10 seconds;
					break;
				case 7:
					if (second % 10 == 9)
						cntDownTime = cntDownTime - 9l;				// - 9 seconds;
					else
						cntDownTime = cntDownTime + 1l;				// + 1 second;
					break;

// must start the count down;
				case 8:
					blinkingDigit = 999;		
			}
		}

		newTime = cntDownTime;
		day = (long)(newTime / 86400l);
		newTime = (long)(newTime % 86400l);	// reuse variable;
		hour = (long)(newTime / 3600l);
		newTime = (long)(newTime % 3600l);	// reuse variable;
		minute = newTime / 60;
		second = newTime % 60;
		
		// build the string containing the formatted time;
		wiseClock.tempBuffer[0]	= '0' + day / 10;
		wiseClock.tempBuffer[1]	= '0' + day % 10;
		wiseClock.tempBuffer[2]	= ':';
		wiseClock.tempBuffer[3]	= '0' + hour / 10;
		wiseClock.tempBuffer[4]	= '0' + hour % 10;
		wiseClock.tempBuffer[5]	= ':';
		wiseClock.tempBuffer[6]	= '0' + minute / 10;
		wiseClock.tempBuffer[7]	= '0' + minute % 10;
		wiseClock.tempBuffer[8]	= ':';
		wiseClock.tempBuffer[9]	= '0' + second / 10;
		wiseClock.tempBuffer[10]= '0' + second % 10;
	
		//	blink the digits that we are setting;
		if (millis() & 0x100)							// changes every 256 milliseconds
		{
			if (blinkingDigit < 8)
			{
				i = blinkingDigit;						// skip colon : seperator;
				if (i > 1)
					++i;
				if (i > 4)
					++i;
				if (i > 7)
					++i;
				wiseClock.tempBuffer[i]	=	' ';
				ht1632_putchar(26, 8, 'G', ORANGE);
				ht1632_putchar(32, 8, 'o', ORANGE);
			}
			else
			{	
				ht1632_putchar(26, 8, ' ', BLACK);
				ht1632_putchar(32, 8, ' ', BLACK);
			}	
		}
		else
		{
			ht1632_putchar(26, 8, 'G', ORANGE);
			ht1632_putchar(32, 8, 'o', ORANGE);
		}

//		for (i=0; i<11; i++)
//			ht1632_putchar(i*6-1, 0, wiseClock.tempBuffer[i], GREEN);

		ht1632_putchar( 1, 0, wiseClock.tempBuffer[0], GREEN);
		ht1632_putchar( 7, 0, wiseClock.tempBuffer[1], GREEN);
		ht1632_putchar(12, 0, ':', GREEN);
		ht1632_putchar(18, 0, wiseClock.tempBuffer[3], GREEN);
		ht1632_putchar(24, 0, wiseClock.tempBuffer[4], GREEN);
		ht1632_putchar(29, 0, ':', GREEN);
		ht1632_putchar(35, 0, wiseClock.tempBuffer[6], GREEN);
		ht1632_putchar(41, 0, wiseClock.tempBuffer[7], GREEN);
		ht1632_putchar(46, 0, ':', GREEN);
		ht1632_putchar(52, 0, wiseClock.tempBuffer[9], GREEN);
		ht1632_putchar(58, 0, wiseClock.tempBuffer[10],GREEN);

			
		if (blinkingDigit == 999)					// Plus key pressed while "Go" was blinking will start the count down;
		{
			startTime = (long)((alarmClock.hour * 3600l) + (alarmClock.minute * 60) + alarmClock.second);
			wiseClock.buttonsInUse = SET_BUTTON;	// the Plus key can now be used to change the brightness;
			freezeTime = false;
			cntDownStartUp = false;

			// (fc, Sep 14, 2013) save the end time in eeprom, to be able to resume countdown later;
			time_t timeNow = makeUnixTime(alarmClock.year, alarmClock.month, alarmClock.day, alarmClock.hour, alarmClock.minute, alarmClock.second);
			saveEndTime(timeNow + cntDownTime);
		}	
	}
	else
	{
		if (freezeTime == false)
		{
			currentTime = (long)((alarmClock.hour * 3600l) + (alarmClock.minute * 60) + alarmClock.second);
			if (currentTime < startTime)
				currentTime = currentTime + long(24l * 3600l);	// correction if we go from 23:59 - > 00:00;
			newTime = cntDownTime - (currentTime - startTime);
			if (newTime	< 0l) 					// in case we missed one or more second increments;
				newTime = 0;

			day = (long)(newTime / 86400l);
			newTime = (long)(newTime % 86400l);
			hour = (long)(newTime / 3600l);
			newTime = (long)(newTime % 3600l);
			minute = newTime / 60;
			second = newTime % 60;
		}
			
		// build the string containing the formatted time;
		wiseClock.tempBuffer[0]	= '0' + day / 10;
		wiseClock.tempBuffer[1]	= '0' + day % 10;
		wiseClock.tempBuffer[2]	= ':';
		wiseClock.tempBuffer[3]	= '0' + hour / 10;
		wiseClock.tempBuffer[4]	= '0' + hour % 10;
		wiseClock.tempBuffer[5]	= ':';
		wiseClock.tempBuffer[6]	= '0' + minute / 10;
		wiseClock.tempBuffer[7]	= '0' + minute % 10;
		wiseClock.tempBuffer[8]	= ':';
		wiseClock.tempBuffer[9]	= '0' + second / 10;
		wiseClock.tempBuffer[10]= '0' + second % 10;

		color = ORANGE;
		if (day == 0)
			color = RED;
		
//		for (i=0; i<11; i++)
//			ht1632_putchar(i*6-1, 0, wiseClock.tempBuffer[i], color);
		
		ht1632_putchar( 1, 0, wiseClock.tempBuffer[0], color);
		ht1632_putchar( 7, 0, wiseClock.tempBuffer[1], color);
		ht1632_putchar(12, 0, ':', color);
		ht1632_putchar(18, 0, wiseClock.tempBuffer[3], color);
		ht1632_putchar(24, 0, wiseClock.tempBuffer[4], color);
		ht1632_putchar(29, 0, ':', color);
		ht1632_putchar(35, 0, wiseClock.tempBuffer[6], color);
		ht1632_putchar(41, 0, wiseClock.tempBuffer[7], color);
		ht1632_putchar(46, 0, ':', color);
		ht1632_putchar(52, 0, wiseClock.tempBuffer[9], color);
		ht1632_putchar(58, 0, wiseClock.tempBuffer[10],color);

		if (wiseClock.setButtonCnt > 0)
		{
			wiseClock.setButtonCnt = 0;
			freezeTime = !freezeTime;	
		}	

		if ((hour == 0) && (newTime == 0)) 					// ready
		{
			if (++loopCnt <= 6)
			{
				if (loopCnt & 1)
				{
					clearDisplay();
					beep();
				}
				else
				{
					ht1632_putchar(26, 8, 'G', RED);
					ht1632_putchar(32, 8, 'o', RED);

//					for (byte i=0; i<11; i++)
//						ht1632_putchar(i*6-1, 0, wiseClock.tempBuffer[i], RED);

					ht1632_putchar( 1, 0, wiseClock.tempBuffer[0], RED);
					ht1632_putchar( 7, 0, wiseClock.tempBuffer[1], RED);
					ht1632_putchar(12, 0, ':', RED);
					ht1632_putchar(18, 0, wiseClock.tempBuffer[3], RED);
					ht1632_putchar(24, 0, wiseClock.tempBuffer[4], RED);
					ht1632_putchar(29, 0, ':', RED);
					ht1632_putchar(35, 0, wiseClock.tempBuffer[6], RED);
					ht1632_putchar(41, 0, wiseClock.tempBuffer[7], RED);
					ht1632_putchar(46, 0, ':', RED);
					ht1632_putchar(52, 0, wiseClock.tempBuffer[9], RED);
					ht1632_putchar(58, 0, wiseClock.tempBuffer[10],RED);

				}
				dly = 250;
			}
			else
			{
				init();								// Start all over again;
			}
		}
		else
		{
			// display a static line instead ("Day:Hour:Min:Sec");
			ht1632_putTinyString(0, 9, "Day Hour Min Sec", GREEN);
//			wiseClock.displayTime(8, false);
		}
	}

#endif

	return(dly);
}


//*********************************************************************************************************
// Read and write the target endtime for countdown (unsigned long, that is 4 bytes, as little endian);
//
time_t CAppCntDown::getEndTime()
{
	time_t endTime = 0UL;
	for (int i=0; i<4; i++)
	{
		endTime += ( (time_t) (wiseClock.readUserSetting(countDownEndTimeLoc+i)) ) << (8*i);
	}
	return endTime;
}

void CAppCntDown::saveEndTime(time_t endTimeInSeconds)
{
	wiseClock.saveUserSetting(countDownEndTimeLoc  , (byte) (endTimeInSeconds));
	wiseClock.saveUserSetting(countDownEndTimeLoc+1, (byte) (endTimeInSeconds>>8));
	wiseClock.saveUserSetting(countDownEndTimeLoc+2, (byte) (endTimeInSeconds>>16));
	wiseClock.saveUserSetting(countDownEndTimeLoc+3, (byte) (endTimeInSeconds>>24));
}



CAppCntDown appCntDown;

#endif
