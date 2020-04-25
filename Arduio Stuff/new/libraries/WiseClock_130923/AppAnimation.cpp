// AppAnimation.cpp
// Show animations contained in anim?.wc3 files
//
//
//*********************************************************************************************************
//*	Edit History, started December, 2011
//*	please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//*	Mar   25,	2012	(rp) first version;
//*	Jun   25,	2012	(mcm) changes to better support multiple displays
//*********************************************************************************************************

#include "UserConf.h"
#ifdef WANT_APP_ANIM

#include "AppAnimation.h"
#include "HT1632.h"
#include "AlarmClock.h"
#include "WiseClock.h"
#include <SdFat.h>


#define MAXFILES 14				// Max. number of anim files, anim0 - anim9 + time00 - time45 = 14						

// Only one display supported for now -- force CHIP_MAX to 4
#undef CHIP_MAX
#define CHIP_MAX 4

void CAppAnimation::init(byte mode)
{
	clearDisplay();
	wiseClock.buttonsInUse	= SET_BUTTON + PLUS_BUTTON;
	wiseClock.plusButtonCnt = 0;	
	wiseClock.setButtonCnt = 0;									// used for starting App;
	appAnimationState = 1;
	quarterlyRunning = false;
	allFiles = false;
}


void CAppAnimation::showQuarter(byte minute)
{
	strcpy_P(wiseClock.tempBuffer, PSTR("time00.wc3"));
	if (minute == 15)
		strcpy_P(wiseClock.tempBuffer, PSTR("time15.wc3"));
	if (minute == 30)
		strcpy_P(wiseClock.tempBuffer, PSTR("time30.wc3"));
	if (minute == 45)
		strcpy_P(wiseClock.tempBuffer, PSTR("time45.wc3"));
		
	if (!wiseClock.openLongFile(wiseClock.tempBuffer))					// Test if file exists;
		return;										// if no file do nothing;
		
	quarterlyRunning = true;
	allFiles = false;
	endIndicator = 0;
	wiseClock.saveCrtApp();
	wiseClock.crtApp = APP_ANIM;								// switch to anim app temporarely;
	wiseClock.pCrtApp = &appAnimation;
	wiseClock.crtPos = 9999;
	appAnimationState = 3;
}


int16_t CAppAnimation::run()
{
	char vBuffer[64];
	byte iArray[sizeof(struct animHeader)];
	byte i, j, k, chip;

	if (appAnimationState == 1)
	{
		if (wiseClock.plusButtonCnt > 1)							
			wiseClock.plusButtonCnt = 0;

		displayStaticLine_P(PSTR("All? "), 0, GREEN);					// All Animations ?  Yes / No;

		if (wiseClock.plusButtonCnt == 0)				
			displayStaticLine_P(PSTR(" Yes "), 8, GREEN);
		else	
			displayStaticLine_P(PSTR(" No  "), 8, GREEN);
			
		if (wiseClock.setButtonCnt == 0)						// wait till Set button pressed;
			return(100);

		wiseClock.setButtonCnt = 0;	

		allFiles = true;
		allFileCount = 0;
		endIndicator = LASTSCREEN;							// to enter the next anim file loop and start with anim1.wc3;
		appAnimationState = 3;

		if (wiseClock.plusButtonCnt == 1)
		{
			allFiles = false;
			wiseClock.plusButtonCnt = getAnimFileDigit();				// used for changing file name anim?.wc3;	
			appAnimationState = 2;
		}	
	}	
		
	if (appAnimationState == 2)
	{
		if (wiseClock.plusButtonCnt > 10)						// file name runs from anim1 -> anim0;
			wiseClock.plusButtonCnt = 1;

		wiseClock.displayTime(8, false);
		strcpy_P(wiseClock.tempBuffer, PSTR("anim1"));
		wiseClock.tempBuffer[4] = '0' + (wiseClock.plusButtonCnt % 10);
		displayStaticLine(wiseClock.tempBuffer, 0, GREEN);

		if (wiseClock.setButtonCnt == 0)						// wait till Set button pressed;
			return(100);
			
		wiseClock.setButtonCnt = 0;							// try again with a different file ?

		strcat_P(wiseClock.tempBuffer, PSTR(".wc3"));

		if (!wiseClock.openLongFile(wiseClock.tempBuffer))				// Test if file exists;
		{
			wiseClock.closeLongFile();
			displayStaticLine_P(PSTR("Wrong"), 0, RED);
			return(2000);
		}
		saveAnimFileDigit(wiseClock.plusButtonCnt);
		endIndicator = 0;
		appAnimationState = 3;
	}	

	if (appAnimationState == 3)
	{	
		setBrightness(3);
		recordStartAppTime();
		delayTime = 0;
		repeatCnt = 0;
		inRepeatCycle = false;
		randomDelayFlag = true;
		randomDelayTime = random(1, 11) * 32;
		repeatPos = 0UL;
		appAnimationState = 4;
#ifdef MULTI_DISPLAY_SCROLL
		clearDisplay();
#endif
	}

				
	if (hasAppTimePassed(delayTime) == false)			// wait till delay time finished;
		return(1);
	
	if (endIndicator & LASTSCREEN)						// Last screen ?
	{
		if (quarterlyRunning == true)
		{
			wiseClock.closeLongFile();
			clearDisplay();
			setBrightness(wiseClock.nBrightness);  
			// return to the app that ran before the animation;
			wiseClock.restoreCrtApp();
			return(1);
		}
		
		randomDelayFlag = true;
		randomDelayTime = random(1, 11) * 32;
		
		if (allFiles == false)
		{
			wiseClock.seekLongFile(0UL);				// restart from beginning of anim file;
		}
		else
		{
			i = 0;
			do
			{
				if (++allFileCount >= MAXFILES)
					allFileCount = 0;
					
				strcpy_P(wiseClock.tempBuffer, PSTR("anim0.wc3"));
				wiseClock.tempBuffer[4] = '0' + allFileCount;
					
				if (allFileCount == 10)
					strcpy_P(wiseClock.tempBuffer, PSTR("time00.wc3"));
				if (allFileCount == 11)
					strcpy_P(wiseClock.tempBuffer, PSTR("time15.wc3"));
				if (allFileCount == 12)
					strcpy_P(wiseClock.tempBuffer, PSTR("time30.wc3"));
				if (allFileCount == 13)
					strcpy_P(wiseClock.tempBuffer, PSTR("time45.wc3"));

				if (i++ >= MAXFILES)
				{
					clearDisplay();
					displayStaticLine_P(PSTR(" No  "), 0, RED);
					displayStaticLine_P(PSTR("File "), 8, RED);
					appAnimationState = 1;
					wiseClock.closeLongFile();
					return(2000);
				}	
			}	
			while (!wiseClock.openLongFile(wiseClock.tempBuffer));		// Test if file can be opened;
		}
	}

	// save start time;
	recordStartAppTime();	
	 
	if (appAnimationState == 4) 
	{
		for (chip = 1; chip <= CHIP_MAX; chip++)
		{
			wiseClock.readFromLongSD(vBuffer, 64);
			if (chip == 1)
			{
				for (k = 0; k < sizeof(struct animHeader); k++)
				{
					j = vBuffer[k];
					iArray[k]  = j & 0xf0;
					vBuffer[k] = j & 0x0f;
				}
			}
#ifdef MULTI_DISPLAY_SCROLL
			byte nchip;
			switch (chip) {
				case 1:
					nchip = 2;
					break;
				case 2:
					nchip = 5;
					break;
				case 3:
					nchip = 4;
					break;
				case 4:
					nchip = 7;
					break;
			}
			ht1632_copyToVideo(nchip, vBuffer);	
#else
			ht1632_copyToVideo(chip, vBuffer);	
#endif
		}
		
		if ((iArray[0] != 0x70)	|| (iArray[1] != 0x90))			// Check if we have a valid anim.wc3 file;
		{
			clearDisplay();
			displayStaticLine_P(PSTR("Ident"), 0, RED);			// This is not an anim.wc3 file !!;
			displayStaticLine_P(PSTR("WRONG"), 8, RED);				
			appAnimationState = 1;
			wiseClock.closeLongFile();
			return(3000);
		}	
		
		delayTime =  iArray[2] << 8;
		delayTime += iArray[3] << 4;
		delayTime += iArray[4];
		delayTime += iArray[5] >> 4;

		if (delayTime != 0)
			randomDelayFlag = false;
			
		if (randomDelayFlag == true)
			delayTime = randomDelayTime;
			
		bIndicator1 = iArray[6] >> 4;					// start brightness value;
		bIndicator2 = iArray[7] >> 4;					// end brightness value;

		endIndicator = iArray[8] >> 4;					// 8 = last screen, 4 = Start repeat cycle, 2 = End repeat cycle;

		if (endIndicator & STARTREPEAT)					// Store Start of repeat Cycle;
		{
			repeatPos = wiseClock.tellLongFile() - 256;		// one screen is 256 bytes;
		}
		
		if ((endIndicator & ENDREPEAT) && (inRepeatCycle == false))	// End of Repeat Cycle detected, store # of repeats;
		{
			repeatCnt =  iArray[9] << 8;
			repeatCnt += iArray[10] << 4;
			repeatCnt += iArray[11];
			repeatCnt += iArray[12] >> 4;
		}
		
		bValue = 0;
		if (bIndicator2 != 0)
		{
			if (bIndicator1 < bIndicator2)
				bValue = 1;

			if (bIndicator1 > bIndicator2)
				bValue = (-1);
		}
	}

	if (bIndicator1 != 0)
		setBrightness(bIndicator1 - 1);   					// 1 - 6  ->  0 - 5

	if (bValue != 0)
	{
		if (bIndicator1 != bIndicator2)
		{
			appAnimationState = 99;						// repeat the same screen with different brightness;
			bIndicator1 += bValue;
			return(1);
		}	
		else
		{		
			appAnimationState = 4;
		}	
	}
	
	if (endIndicator & ENDREPEAT)
	{
		if (inRepeatCycle == true)
		{
			if (--repeatCnt < 2)
				inRepeatCycle = false;
			else
				wiseClock.seekLongFile(repeatPos);		// reposition to start of repeat cycle;
		}
		else
		{	
			inRepeatCycle = true;
			wiseClock.seekLongFile(repeatPos);			// reposition to start of repeat cycle;
		}
	}
	return(delayTime);
}	

//*********************************************************************************************************
// Read and write param indicating if quarterly Animation is on;
boolean CAppAnimation::getAnim()
{
	return boolean( wiseClock.readUserSetting(animLoc) );
}

//*********************************************************************************************************
void CAppAnimation::saveAnim()
{
	wiseClock.saveUserSetting( animLoc, isAnimEnabled);
}

//*********************************************************************************************************
// Read and write last digit of anim?.bin file;
byte CAppAnimation::getAnimFileDigit()
{
	return byte (wiseClock.readUserSetting( animFileLoc ));
}

//*********************************************************************************************************
void CAppAnimation::saveAnimFileDigit(byte animFileDigit)
{
	wiseClock.saveUserSetting(animFileLoc, animFileDigit);
}

CAppAnimation appAnimation;
#endif
