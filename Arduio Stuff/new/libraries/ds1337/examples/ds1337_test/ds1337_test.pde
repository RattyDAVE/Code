#include <WProgram.h>
#include <Wire.h>
#include <DS1337.h>


void setup()
{
Serial.begin(9600);
  RTC.Init();

Serial.print("Clock exists: ");
Serial.println(RTC.clockExists, DEC);


	// Set the clock using a unix time stamp (date -u +'%s')
	uint32_t unixTime = RTC.calculateUTS(2010, 4, 17, 12, 25, 32);
	RTC.clockSetWithUTS(unixTime, false); //Sun Oct 14 15:27:48 EDT 2007
/*
RTC.clockSet(DS1337_HR, 12);
RTC.clockSet(DS1337_HR, 11);
RTC.clockSet(DS1337_HR, 32);
*/

	// Setup a callback function for the integrity check
	// See below for the callback functions
//	RTC.clockIntegrityCallback(integrityCallback);
/*
	// Enable the Square Wave @ 1Hz on INTB
	RTC.sqwSetRate(DS1337_SQW_1HZ);
	RTC.sqwEnable();

	// Setup the first alarm
	RTC.alarmSelect(0); // Switch to alarm 2
	RTC.alarmSet(RTC_SEC,	30);
	RTC.alarmSet(RTC_MIN,	18);
	RTC.alarmSet(RTC_HR,	0);
	RTC.alarmSet(RTC_DOW,	0);

	// Match mode: every minutes and seconds
	RTC.alarmSet(DS1337_ALARM_MODE, DS1337_ALARM_MCH_MINSEC);

	// Setup callback function for alarm 1
//	RTC.alarmSetCallback(alarmCallback1);

	// Setuo the external interrupt on INTA for alarm1
	RTC.alarmSetInterrupt();

	RTC.alarmSelect(1); // Switch to alarm 2

	// Match mode: once per minute at 00s
	RTC.alarmSet(DS1337_ALARM_MODE, DS1337_ALARM_PER_MIN);

	// Setup callback function for alarm 2
//	RTC.alarmSetCallback(alarmCallback2);

	// Setup the interrupt for alarm 2
	// If the square wave is enabled, it will output on INTA
	// if the SQW is disabled the interrupt will happen on INTB
	RTC.alarmSetInterrupt();
*/
RTC.clockStart();
}

void loop()
{
  int hour = RTC.clockGet(DS1337_HR, true);
  int min  = RTC.clockGet(DS1337_MIN, true);
  int sec  = RTC.clockGet(DS1337_SEC, true);
  
Serial.print("Time is: ");
Serial.print(hour);
Serial.print(":");
Serial.print(min);
Serial.print(":");
Serial.println(sec);

/*
  RTC.clockRead();

Serial.print("Time is: ");
Serial.print(RTC.clockGetHour());
Serial.print(":");
Serial.print(RTC.clockGetMin());
Serial.print(":");
Serial.println(RTC.clockGetSec());
Serial.print("Date is: ");
Serial.print(RTC.clockGetDate());
Serial.print("/");
Serial.print(RTC.clockGetMonth());
Serial.print(", ");
Serial.println(RTC.clockGetDow());
*/

  delay(3000);
}

// Note that the alarm registers will stay high
// until these function are finished executing.
void alarmCallback1(void)
{
	Serial.print("alarm 1 triggerd");
	return;
}
void alarmCallback2(void)
{
	Serial.print("alarm 2 triggerd");
	return;
}

void integrityCallback(void)
{
	Serial.print("Oscillator integrity fault!");

	// If the fault flag is high the clock is stopped
	// so in this function we can fix the time and start the clock again
	RTC.clockStart();
	return;
}
 

