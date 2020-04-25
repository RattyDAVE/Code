// AppWords.h

#ifndef _APP_WORDS_H_
#define _APP_WORDS_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif
#include "WiseClock.h"

#include "AppBase.h"


#define MAX_WORDS1PARAMLENGTH 21
#define MAX_WORDS2PARAMLENGTH 51


class CAppWords : public CAppBase
{
private:
	boolean startUpAppWords;
	char hourNumber[MAX_WORDS1PARAMLENGTH];
	char hourNumberPlusOne[MAX_WORDS1PARAMLENGTH];
	char amString[MAX_WORDS1PARAMLENGTH];
	char pmString[MAX_WORDS1PARAMLENGTH];
	char tString[MAX_WORDS2PARAMLENGTH];
	char tPlusOneString[MAX_WORDS2PARAMLENGTH];
	byte prevMinute;
	byte prevHour;
	byte prevDay;
	byte prevMonth;
	byte wordsFileNum;
	int  timeTextLength;
	char fullText[MAX_MSG_LEN + 5];
	
public:
	virtual void init(boolean start=0);
	virtual int16_t run();
	
	// (fc, Sep 1, 2013) moved from WiseClock.cpp;
	void showTimeWithWords(char *mText);
	void saveWordsFileDigit(byte wordsFileDigit);
	byte getWordsFileDigit();
	boolean openWordsFile(byte wn);

private:
	void getDateParams();
	boolean get1Param(char param[], int num, char buf[], char* value);
	void getMinuteParams();
	void createTimeString();
	void fillTimeString(char SDdata[], char fulltext[]);
	void subStr(char src[], char dst[], int first, int len);
};


extern CAppWords appWords;


#endif  // _APP_WORDS_H_


