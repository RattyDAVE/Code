// AppWords.cpp
// Show time with words using word?.txt parameter file
//
// There is not enough ram available to read all the parameters from the word?.txt file into memory.
// But we should minimize the reading of the file on the SD card as this stops/slows down the display
// Therefor the keywords in the word?.txt file should be in a defined sequence
//
//*********************************************************************************************************
//*	Edit History, started October, 2011
//*	please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//*	Oct   15,	2011	(rp) first version;
//*	Apr   29,	2012	(mcm) converted to use new FAT32 library
//*	May   28,	2012	(mcm) restructure to eliminate delay() calls
//*	Jun   25,	2012	(mcm) new args to displayTime()
//*
//*********************************************************************************************************

#include "UserConf.h"
#ifdef WANT_APP_WORDS

#include "AppWords.h"
#include "TimeUtils.h"
#include "SdBaseFile.h"
#include "HT1632.h"
#include "AlarmClock.h"
#include "WiseClock.h"


void CAppWords::init(boolean start /*=0*/)
{
	clearDisplay();
	wiseClock.buttonsInUse	= SET_BUTTON + PLUS_BUTTON;
	wiseClock.plusButtonCnt = getWordsFileDigit();	// used for changing file name word?.txt;
	wiseClock.setButtonCnt = 0;									// used for starting App;
	prevMinute = -1;
	prevHour = -1;
 	prevDay = -1;
	startUpAppWords = start;
	fullText[0] = 0x0;
	timeTextLength = 0x0;
	wordsFileNum = wiseClock.plusButtonCnt % 10;
}


int16_t CAppWords::run()
{
	if (startUpAppWords == true)
	{
		if (wiseClock.plusButtonCnt > 10)						// file name runs from word1 -> word0;
			wiseClock.plusButtonCnt = 1;

		wiseClock.displayTime(8, false);
		strcpy_P(wiseClock.tempBuffer, PSTR("word1"));
		wiseClock.tempBuffer[4] = '0' + (wiseClock.plusButtonCnt % 10);
		displayStaticLine(wiseClock.tempBuffer, 0, GREEN);

		if (wiseClock.setButtonCnt == 0)						// wait till Set button pressed;
			return(100);

		if (!openWordsFile(wiseClock.plusButtonCnt % 10))			// Test if file exists;
		{
			displayStaticLine_P(PSTR("Wrong"), 0, RED);
			wiseClock.setButtonCnt = 0;						// try again with a different word file ?
			return(2000);
		}
		saveWordsFileDigit(wiseClock.plusButtonCnt);
		wordsFileNum = wiseClock.plusButtonCnt % 10;
		ht1632_plot	(15, 15, RED);							// signal update mode;
		getMinuteParams();								// get hour number, am/pm and the t?? string; 
		wiseClock.plusButtonCnt = 0;
		wiseClock.setButtonCnt = 0;
		wiseClock.buttonsInUse = 0;
		displayStaticLine_P(PSTR("      "), 8, BLACK);
		startUpAppWords = false;	
	}

	if (wiseClock.isLineInProgress())						// Not at end of message yet;
	{
		showTimeWithWords(fullText);
		return(100);
	}
	
	if (prevMinute != alarmClock.minute)						// get new time once a minute;
	{
		ht1632_plot	(15, 15, GREEN);					// signal update mode;
		if (prevHour != alarmClock.hour)					// if it is the full hour reopen word file;
		{
			openWordsFile(wordsFileNum);				// Test if file exists;
			getMinuteParams();						// get new N1, A1 and Txx parameters;
		}	
		prevMinute = alarmClock.minute;
		prevHour = alarmClock.hour;
		openWordsFile(wordsFileNum);					// Test if file exists;
		createTimeString();							// get new time info into fullText string;
		ht1632_plot	(15, 15, BLACK);
	}
	
	if ((alarmClock.day != prevDay) || (alarmClock.month != prevMonth))
	{
		ht1632_plot	(15, 15, ORANGE);					// signal update mode;
		prevDay = alarmClock.day; 
		prevMonth = alarmClock.month;
		openWordsFile(wordsFileNum);					// Test if file exists;
		getDateParams();							// get date info into fullText string;
		ht1632_plot	(15, 15, BLACK);
	}

	showTimeWithWords(fullText);
	return(100);
}


void CAppWords::getMinuteParams()
{   
	char temp[10];
	byte hour24, h;

	hourNumber[0] = 0x0;
	hourNumberPlusOne[0] = 0x0;
	amString[0] = 0x0;
	pmString[0] = 0x0;
	tString[0] = 0x0;
	tPlusOneString[0] = 0x0;

	while (true)
	{
		if (wiseClock.readLineFromShortSD(wiseClock.crtBuffer, MAX_MSG_LEN) == false)							// end of file or no file ?
		{
			return; 
		}
		if (wiseClock.isShortTrunc || wiseClock.isShortComment)
			continue;
		if (strlen(wiseClock.crtBuffer) < 2)
			continue;

		hour24 = alarmClock.hour;

		if ((hour24 < 13) || (alarmClock.is24hourModeEnabled == false))			// Not 24 hour mode;
		{
			h = hour24 % 12;
			if (h == 0)
				h = 12;
			if (get1Param("N1: ", h - 1, wiseClock.crtBuffer, &hourNumber[0]))			// get N1: parameter = hour number;
			{
				if (++h > 12)
					h = 1;
				if (get1Param("N1: ", h - 1, wiseClock.crtBuffer, &hourNumberPlusOne[0]))	// get N1: parameter = hour + 1 number;
				{
					continue;
				}	
			}
		}
		else
		{
			h = hour24;
			if (h == 23)
			{
				if (get1Param("N1: ", 11, wiseClock.crtBuffer, &hourNumberPlusOne[0]))		// get N1: first before gettinh N2: value;
					continue;
			}
			if (get1Param("N2: ", h - 13, wiseClock.crtBuffer, &hourNumber[0]))			// get N2: parameter = hour number;
			{
				if (h != 23)
				{
					if (get1Param("N2: ", h - 12, wiseClock.crtBuffer, &hourNumberPlusOne[0]))
						continue;
				}		
			}
		}

		if (get1Param("A1: ", 0, wiseClock.crtBuffer, &amString[0]))					// get AM string;
		{
			if (get1Param("A1: ", 1, wiseClock.crtBuffer, &pmString[0]))				// get PM string
				continue;
		}
		
		strcpy_P(temp, PSTR("T00: "));
		if (hour24 == 23)
		{
			temp[1] = '0';
			temp[2] = '0';
			if (get1Param(temp, 0, wiseClock.crtBuffer, &tPlusOneString[0]))			// get T00 string first then T23;
				continue;
		}		
			
		temp[1] = '0' + hour24 / 10;
		temp[2] = '0' + hour24 % 10;
		if (get1Param(temp, 0, wiseClock.crtBuffer, &tString[0]))					// get T?? string;
			continue;
	
		h = hour24 + 1;
		if (h < 24)
		{
			temp[1] = '0' + h / 10;
			temp[2] = '0' + h % 10;
			if (get1Param(temp, 0, wiseClock.crtBuffer, &tPlusOneString[0]))
				break;										// last parameter found so return;
		}
	}	
}			
			

boolean CAppWords::get1Param(char param[], int num, char buf[], char* value)
{
	int k, j, i;
	char tempS[10];
	subStr(buf, tempS, 0, strlen(param));					// Is the line in the buffer the one we are looking for ?; 
	if (strcmp(param, tempS) == 0)
	{
		j = 0;
		i = strlen(param);
		while ((buf[i] != 0x0) && (j <= num))
		{
			if (buf[i] == '|')
			{
				++j;
				++i;
				continue;
			}	
			if (j == num)
				*value++ = buf[i];				// copy result from string in buffer;
			++i;
		}
		*value = 0x0;
		if (j >= num)
		{
			return true;
		}	
	}
	return false;	
}

#define MINUTE_KEYWORD_LENGTH 5					// Length of "M00: " keyword;

void CAppWords::createTimeString()
{   
	char tempDateString[MAX_MSG_LEN + 5];
	char temp[10];
	char keyword[10];
	byte minute, hour24;

	tempDateString[0] = 0x0;
	
	subStr(fullText, tempDateString, timeTextLength, strlen(fullText) - timeTextLength);	// save date text temporarely;
	
	strcpy_P(fullText, PSTR("     "));				// start with 5 spaces;
	minute = alarmClock.minute;
	hour24 = alarmClock.hour;
	
	while (true)
	{
		if (wiseClock.readLineFromShortSD(wiseClock.crtBuffer, MAX_MSG_LEN) == false)							// end of file or no file ?
		{
			return; 
		}
		if (wiseClock.isShortTrunc || wiseClock.isShortComment)
			continue;
		if (strlen(wiseClock.crtBuffer) < 2)
			continue;
						
/* Get amount of free memory and dump in fullText buffer to display;
		int j = availableMemory();
		byte k = 0;
		fullText[k] = 'M';
		int l = 1000;
		while (l > 0)
		{
			fullText[++k] = '0' + j / l;
			j = j % l;
			l = l / 10;
		}
		fullText[++k] = ' ';
		fullText[++k] = 0x0;
*/

		subStr(wiseClock.crtBuffer, keyword, 0, MINUTE_KEYWORD_LENGTH);				// build keyword string 5 long + null character;

		if (minute == 0)									// if it is the full hour get the hour string;
		{
			strcpy_P(temp, PSTR("H00: "));	
			temp[1] = '0' + (hour24 / 10);
			temp[2] = '0' + (hour24 % 10);
		}
		else
		{
			strcpy_P(temp, PSTR("M00: "));							// get the new minute string;
			temp[1] = '0' + (minute / 10);
			temp[2] = '0' + (minute % 10);
		}

		if (strcmp(temp, keyword) == 0)
		{
			fillTimeString(wiseClock.crtBuffer, fullText);
			break;			
		}	
	}
	timeTextLength = strlen(fullText);
	if (MAX_MSG_LEN >= timeTextLength + strlen(tempDateString))
		strcat(fullText, tempDateString);							// date info goes at end time info;
}


void CAppWords::fillTimeString(char SDdata[], char fullText[])
{
	byte k = strlen(fullText);
	byte i = MINUTE_KEYWORD_LENGTH;

	while ((SDdata[i] != 0) && (i < MAX_MSG_LEN))
	{
		if (SDdata[i] == '%')
		{
			if (SDdata[i + 1] == 'H')							// insert Hour;
			{
				i = i + 2;
				fullText[k] = 0x0;
				if ((SDdata[i] == '+') && (SDdata[i + 1] == '1'))
				{
					i = i + 2;
					strcat(fullText, hourNumberPlusOne);
					k = k + strlen(hourNumberPlusOne);
				}
				else							
				{
					strcat(fullText, hourNumber);
					k = k + strlen(hourNumber);
				}			
			}

			if (SDdata[i + 1] == 'T')							// insert T??: text;
			{
				i = i + 2;
				fullText[k] = 0x0;
				if ((SDdata[i] == '+') && (SDdata[i + 1] == '1'))
				{
					i = i + 2;
					strcat(fullText, tPlusOneString);
					k = k + strlen(tPlusOneString);
				}	
				else
				{
					strcat(fullText, tString);
					k = k + strlen(tString);
				}
			}

			if (SDdata[i + 1] == 'A') 							// insert AM / PM text;
			{
				i = i + 2;
				if (alarmClock.is24hourModeEnabled == false)				// No AM / PM in 24 hour mode;
				{
					fullText[k] = 0x0;
					if (alarmClock.hour > 11)
					{
						strcat(fullText, pmString);
						k = k + strlen(pmString);
					}
					else
					{
						strcat(fullText, amString);
						k = k + strlen(amString);
					}
				}	
			}	
		}
		else
		{
			fullText[k++] = SDdata[i++];					
		}
	}
	fullText[k] = 0x0;
}


void CAppWords::getDateParams()
{
	int i, k;
	char work[10];
	char wday[MAX_WORDS1PARAMLENGTH];
	char nmonth[MAX_WORDS1PARAMLENGTH];
	char nday[MAX_WORDS1PARAMLENGTH];

	fullText[timeTextLength] = 0x0;									// remove previous datestring;
	strcat(fullText, " ");										// add 1 space behind the time text;
		
	wday[0] = 0x0;
	nmonth[0] = 0x0;
	nday[0] = 0x0;

	while (true)
	{
		if (wiseClock.readLineFromShortSD(wiseClock.crtBuffer, MAX_MSG_LEN) == false)							// end of file or no file ?
		{
			return; 
		}
		if (wiseClock.isShortTrunc || wiseClock.isShortComment)
			continue;
		if (strlen(wiseClock.crtBuffer) < 2)
			continue;

		if (get1Param("DW: ", alarmClock.dow - 1, wiseClock.crtBuffer, &wday[0]))		// get day name;
			continue;

		if (get1Param("DM: ", alarmClock.month - 1, wiseClock.crtBuffer, &nmonth[0]))		//get month name;
			continue;

		strcpy_P(work, PSTR("D : "));
		i = (alarmClock.day - 1) / 10;
		work[1] = '1' + i;
		i = (alarmClock.day - 1) % 10;

		if (get1Param(work, i, wiseClock.crtBuffer, &nday[0]))					// get day of the month number;
			continue;
			
		if ((wiseClock.crtBuffer[0] == 'D') && (wiseClock.crtBuffer[1] == 'T') && (wiseClock.crtBuffer[2] == ':') && (wiseClock.crtBuffer[3] == ' '))
		{
			i = 4;
			k = strlen(fullText);								// we already have 1 space;
			while ((wiseClock.crtBuffer[i] != 0x0) && (i < MAX_MSG_LEN))
			{
				if (wiseClock.crtBuffer[i] == '%')
				{
					++i;
					if ((wiseClock.crtBuffer[i] == 'D') && (wiseClock.crtBuffer[i + 1] == 'W'))		// Found %DW
					{
						i = i + 2;
						fullText[k] = 0x0;
						strcat(fullText, wday);
						k = k + strlen(wday);
					}	
					if ((wiseClock.crtBuffer[i] == 'D') && (wiseClock.crtBuffer[i + 1] == 'M'))		// Found %DM
					{
						i = i + 2;
						fullText[k] = 0x0;
						strcat(fullText, nmonth);
						k = k + strlen(nmonth);
					}	
					if ((wiseClock.crtBuffer[i] == 'D') && (wiseClock.crtBuffer[i + 1] == 'N'))		// Found %DN
					{
						i = i + 2;
						fullText[k] = 0x0;
						strcat(fullText, nday);
						k = k + strlen(nday);
					}	
				}
				else
				{
					fullText[k++] = wiseClock.crtBuffer[i++];					
				}	
			}	
			break;
		}
	}	
	fullText[k] = ' ';									// add 1 space at the end;
	fullText[k + 1] = 0x0;
}	


void CAppWords::subStr(char src[], char dst[], int first, int len)
{
	int i;
	for(i = 0; (i < len) && (src[i + first] != 0x0); ++i)
		dst[i] = src[i + first];
	dst[i] = 0x0;
	return;
}


boolean CAppWords::openWordsFile(byte wn)
{
	strcpy_P(wiseClock.crtBuffer, PSTR("word1.txt"));
	wiseClock.crtBuffer[4] = '0' + wn;
	if (wiseClock.openShortFile(wiseClock.crtBuffer, O_READ)) {
		wiseClock.crtBuffer[0] = 0;
		wiseClock.resetCrtPos();
		return true;
	} else {
		return false;
	}
}


//*********************************************************************************************************
// Read and write last digit of word?.txt file;
byte CAppWords::getWordsFileDigit()
{
	return byte (wiseClock.readUserSetting( wordsFileLoc ));
}

//*********************************************************************************************************
void CAppWords::saveWordsFileDigit(byte wordsFileDigit)
{
	wiseClock.saveUserSetting(wordsFileLoc, wordsFileDigit);
}

//*********************************************************************************************************
// Show time with moving text;
//
void CAppWords::showTimeWithWords(char *mText)
{
	byte day;
	
	if (wiseClock.isLineInProgress())
	{
		if (wiseClock.largeTextFont == false)		// show date on bottom of display;
		{
			strcpy(wiseClock.tempBuffer, dayName[alarmClock.dow]);
			ht1632_putchar( 1, 8, wiseClock.tempBuffer[0], GREEN|SMALL_CHAR);
			ht1632_putchar( 7, 8, wiseClock.tempBuffer[1], GREEN|SMALL_CHAR);
			ht1632_putchar(13, 8, wiseClock.tempBuffer[2], GREEN|SMALL_CHAR);
			day = alarmClock.day;
			ht1632_putchar(21, 8, (day > 9) ? '0' + day / 10 : '0' + day, GREEN|SMALL_CHAR);
			ht1632_putchar(26, 8, (day > 9) ? '0' + day % 10 : ' ',       GREEN|SMALL_CHAR);
		}
	}
	else
	{
		if (wiseClock.showExtraMessages == 0)
		{
			wiseClock.showExtraMessages = 1;
			strcpy(wiseClock.crtBuffer, mText);
			wiseClock.resetCrtPos();
			wiseClock.crtColor = wiseClock.getColor();
		}
		else
		{
			wiseClock.showExtraMessages = 0;
			wiseClock.fetchExtraInfo();
		}
	}
}


CAppWords appWords;

#endif
