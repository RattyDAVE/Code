// Buttons.cpp

#include "Buttons.h"
#include "AlarmClock.h"
#include "WiseClock.h"


// button debouncing adapted from http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210559123/7;
#define BOUNCE_TIME_BUTTON		600	// bounce time in ms for the menu button;


// last time the respective button was pressed; used for debouncing;
volatile unsigned long timeBtnMenu	=	0;
volatile unsigned long timeBtnSet 	=	0;
volatile unsigned long timeBtnPlus	=	0;



//*********************************************************************************************************
void setupButtons()
{
	pinMode(BUTTON_MENU_PIN,	INPUT);
	pinMode(BUTTON_SET_PIN,		INPUT);
	pinMode(BUTTON_PLUS_PIN,	INPUT);
}



//*********************************************************************************************************
void checkButtons(unsigned long ms)
{
	// check buttons;

	if (digitalRead(BUTTON_MENU_PIN) == LOW)
	{
		alarmClock.isAlarmSoundOn	=	false;

	        // debouncing;
	        if ((long)(ms - timeBtnMenu) >= 0)
		{
			wiseClock.processButtonMenu();
			timeBtnMenu = ms + BOUNCE_TIME_BUTTON;
		}
		return;
	}

	if (digitalRead(BUTTON_SET_PIN) == LOW)
	{
		alarmClock.isAlarmSoundOn	=	false;
	        // debouncing;
	        if ((long)(ms - timeBtnSet) >= 0)
		{
			wiseClock.processButtonSet();
			timeBtnSet = ms + BOUNCE_TIME_BUTTON;
		}
		return;
	}

	if (digitalRead(BUTTON_PLUS_PIN) == LOW)
	{
		alarmClock.isAlarmSoundOn	=	false;

	        // debouncing;
	        if ((long)(ms - timeBtnPlus) >= 0)
		{
			wiseClock.processButtonPlus();
			timeBtnPlus = ms + BOUNCE_TIME_BUTTON;
		}
		return;
	}
}	


//*********************************************************************************************************
boolean readButtons()
{
	if ((digitalRead(BUTTON_MENU_PIN) == LOW) || (digitalRead(BUTTON_SET_PIN) == LOW) || (digitalRead(BUTTON_PLUS_PIN) == LOW))
	{
		return true;
	}	
	else
	{
		return false;
	}
}
