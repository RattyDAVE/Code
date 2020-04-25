/*
  DS1337.h - library for DS1337 I2C Real Time Clock
*/

#ifndef DS1337_h
#define DS1337_h

// include types & constants of Wiring core API
#include <WConstants.h>

// include types & constants of Wire ic2 lib
#include <../Wire/Wire.h>

#include "rtcConfig.h"


#define	DS1337_DEBUG
//#define DS1337_USE_OSC_INTEGRITY
//#define DS1337_USE_ALARMS_CALLBACK
#define DS1337_USE_ALARMS	
#define DS1337_USE_SET_UTS
#define DS1337_USE_GET_UTS
#define DS1337_USE_SQW_OUTPUT
#define DS1337_USE_ALARM_INTERRUPTS
#define DS1337_USE_BCD_CLOCKSET


#define voidFuncPtr (void(*)())



/**
 * Define the position of the RTC buffer values
**/
#define DS1337_SEC						0
#define DS1337_MIN						1
#define DS1337_HR						2
#define DS1337_DOW						3
#define DS1337_DATE 					4
#define DS1337_MTH						5
#define DS1337_YR						6
#define DS1337_CNTY						7

// For use externally
#define RTC_SEC							DS1337_SEC
#define RTC_MIN							DS1337_MIN
#define RTC_HR							DS1337_HR
#define RTC_DOW							DS1337_DOW
#define RTC_DATE						DS1337_DATE
#define RTC_MTH							DS1337_MTH
#define RTC_YR							DS1337_YR
#define RTC_CNTY						DS1337_CNTY

/**
 * Define the DS1337 I2C addresses
**/
#ifndef DS1337_ADDR
#define DS1337_ADDR					B01101000
#endif

/**
 * Define registers and bit masks
**/
#define DS1337_LO_BCD					B00001111
#define DS1337_HI_BCD					B01110000

#define DS1337_HI_SEC					B01110000
#define DS1337_HI_MIN					B01110000
#define DS1337_HI_HR					B00110000
#define DS1337_LO_DOW					B00000111
#define DS1337_HI_DATE					B00110000
#define DS1337_HI_MTH					B00010000
#define DS1337_LO_CNTY					B10000000
#define DS1337_HI_YR					B11110000

#define DS1337_ARLM1					0x07
#define DS1337_ARLM1_LO_SEC				B00001111
#define DS1337_ARLM1_HI_SEC				B01110000
#define DS1337_ARLM1_LO_MIN				B01110000
#define DS1337_ARLM1_HI_MIN				B00001111

#define DS1337_SP						0x0E
#define	DS1337_SP_EOSC					B10000000
#define	DS1337_SP_RS2					B00010000
#define	DS1337_SP_RS1					B00001000
#define	DS1337_SP_INTCN					B00000100
#define	DS1337_SP_A2IE					B00000010
#define	DS1337_SP_A1IE					B00000001

#define DS1337_STATUS					0x0F
#define DS1337_STATUS_OSF				B10000000
#define DS1337_STATUS_A2F				B00000010
#define DS1337_STATUS_A1F				B00000001

// Alarm registers and masks
#define DS1337_ALARM1					0x07
#define DS1337_ALARM2					0x0B

#define	DS1337_ALARM_DT_MASK			B01000000
#define DS1337_ALARM_MASK				B10000000

#define DS1337_ALARM_MODE				4
#define DS1337_ALARM_DT					5

/**
 * Match Day of the week or match date
**/
#define DS1337_ALARM_DT_DOW				true
#define DS1337_ALARM_DT_DATE			false

/**
 * Alarm 1: Every second
 * Alarm 2: Every minute (at 00s)
**/
#define DS1337_ALARM_PERA				B00001111
#define DS1337_ALARM_PER_SEC			DS1337_ALARM_PERA	/* Used for alarm 1 only*/
#define DS1337_ALARM_PER_MIN			DS1337_ALARM_PERA	/* Used for alarm 2 only*/

/**
 * Alarm 1: Match second
 * Alarm 2: Match minute
**/
#define DS1337_ALARM_MCH_SEC			B00001110			/* Used for alarm 1 only */
#define DS1337_ALARM_MCH_MIN			B00001100			/* Used for alarm 2 only */

/**
 * Alarm 1: Match minutes and seconds
 * Alarm 2: Match hours and minutes
**/
#define DS1337_ALARM_MCH_MINSEC			B00001100			/* Used for alarm 1 only */
#define DS1337_ALARM_MCH_HRMIN			B00001000			/* Used for alarm 2 only */

/**
 * Alarm 1: Match hour, minute and second
**/
#define DS1337_ALARM_MCH_HRMINSEC		B00001000			/* Used for alarm 1 only */

/**
 * Alarm 1: Match date, hour, minute, second
 * Alarm 2: Match da,te hour, minute
**/
#define DS1337_ALARM_MCH_DATEHRMINSEC	B00000000			/* Used for alarm 1 only */
#define DS1337_ALARM_MCH_DATEHRMIN		B00000000			/* Used for alarm 2 only */

/**
 * Alarm 1: Match day of the week, hour, minute, second
 * Alarm 2: Match day of the week, hour, minute
**/
#define DS1337_ALARM_MCH_DOWHRMINSEC	B10000000			/* Used for alarm 1 only */
#define DS1337_ALARM_MCH_DOWHRMIN		B10000000			/* Used for alarm 2 only */

// Alarm mode masks
#define DS1337_ALARM2_MODE_MASK			B00001000
#define	DS1337_ALARM_M1					B00000001
#define	DS1337_ALARM_M2					B00000010
#define	DS1337_ALARM_M3					B00000100
#define	DS1337_ALARM_M4					B00001000

// Alarm interrupt bitmask
#define DS1337_ALARM_INT1				B00000001
#define DS1337_ALARM_INT2				B00000010

// Square Wave output masks
#define DS1337_SQW_INTCN				B00000100
#define DS1337_SQW_RS1					B00001000
#define DS1337_SQW_RS2					B00010000

// Square Wave output modes
#define DS1337_SQW_RS					B00011000
#define DS1337_SQW_1HZ					B00000000
#define DS1337_SQW_4096KHZ				DS1337_SQW_RS1
#define DS1337_SQW_8192KHZ				DS1337_SQW_RS2
#define DS1337_SQW_OSC					DS1337_SQW_RS1 | DS1337_SQW_RS2

/**
 * Macros
**/
#define clockStop()						setRegister(DS1337_SP, DS1337_SP_EOSC)

#define getRegisterSP()					getRegister(DS1337_SP)
#define getRegisterStatus()				getRegister(DS1337_STATUS)

#define getRegisterBit(reg, bitMask)	(getRegister(reg) & bitMask) && bitMask

#ifndef ISLEAP
#define ISLEAP(y)						((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)
#endif

#ifndef BCDTOBIN
// This marco is for internal use only!
#define BCDTOBIN(index, hi)				(10*((rtc_bcd[index] & hi)>>4))+(rtc_bcd[index] & DS1337_LO_BCD)
#endif

#ifndef BINTOBCD
#define BINTOBCD(val) 					((((val)/10)<<4) + (val)%10)
#endif

/**
 * getUTS: Macro for calculateUTS
 * returns the time as a unix time stamp
 * This function doesn't take into account having DST set or not!
**/
#ifdef DS1337_USE_GET_UTS
#define	getUTS(refresh)					calculateUTS(	RTC.clockGet(DS1337_YR, true), RTC.clockGet(DS1337_MTH, false), \
														RTC.clockGet(DS1337_DATE, false), RTC.clockGet(DS1337_HR, false), \
														RTC.clockGet(DS1337_MIN, false), RTC.clockGet(DS1337_SEC, false) \
													)
#endif

/**
 * Use this macro to the time from a unix time stamp
**/
/*
#if defined(DS1337_USE_SET_UTS)
#define clockSet(UTS)					clockSetWithUTS(UTS, false)
#endif
*/


/**
 * Macros for getting time values without refreshing the RTC buffer
**/
#define clockGetSec()					clockGet(DS1337_SEC, false)
#define clockGetMin()					clockGet(DS1337_MIN, false)
#define clockGetHour()					clockGet(DS1337_HR, false)
#define clockGetDate()					clockGet(DS1337_DATE, false)
#define clockGetMonth()					clockGet(DS1337_MTH, false)
#define clockGetDow()					clockGet(DS1337_DOW, false)

/**
 * Macros for getting time values refreshing the RTC buffer before each read.
**/
#define clockGetRSec()					clockGet(DS1337_SEC, true)
#define clockGetRMin()					clockGet(DS1337_MIN, true)
#define clockGetRHour()					clockGet(DS1337_HR, true)
#define clockGetRDate()					clockGet(DS1337_DATE, true)
#define clockGetRMonth()				clockGet(DS1337_MTH, true)
#define clockGetRDow()					clockGet(DS1337_DOW, true)


#if defined(DS1337_USE_ALARMS_CALLBACK) || defined(DS1337_USE_OSC_INTEGRITY)
/**
 * Holds the pointer to callback functions
**/
//volatile static (void (DS1337callbackFunc*)())[3];
volatile static voidFuncPtr DS1337callbackFunc[3];
#endif

// library interface description
class DS1337
{
	// user-accessible "public" interface
	public:
		/**
		 * clockExists: keeps track of the whether or not the RTC exists
		**/
		bool		clockExists;
		
		/**
		 * Class constructor
		**/
		DS1337();
		
		/**
		 * Init: initializes the clock
		 * If the I2C scan mod is available, it'll verify the RTC is reachable
		**/
		int8_t		Init(void);
		
		/**
		 * clockStart: starts the oscillator and reset the fault flag
		**/
		void		clockStart(void);
	#ifdef DS1337_USE_OSC_INTEGRITY
		/**
		 * chockSetIntegrityCallback: allow setting the callback function 
		 * for the oscillator fault check.
		**/
		void		clockIntegrityCallback(void (*)(void));
	#endif
	#if defined(DS1337_USE_ALARMS_CALLBACK) || defined(DS1337_USE_OSC_INTEGRITY)
		/**
		 * clockChecks: performs various clock checks such as integrity and alarms
		 * Will trigger the 
		**/
		void		clockChecks(void);
	#endif

		/**
		 * Read the RTC BCD buffer to the clock
		**/
		void		clockRead(void);
		

		/**
		 * setRegister: sets a register bit fromt he register number and bitmask
		**/
		void		setRegister(uint8_t, uint8_t);
		
		/**
		 * unsetRegister: unsets a register bit fromt he register number and bitmask
		**/
		void		unsetRegister(uint8_t, uint8_t);
		
		/**
		 * getRegister: returns the specified register
		**/
		uint8_t		getRegister(uint8_t);
		
		/**
		 * clockGet: fills an array with the current time data
		**/
		void		clockGet(uint16_t *);
		
		/**
		 * clockGet: gets a specific item from the clock buffer
		 * use the second param to specify a buffer refresh
		**/
		uint16_t	clockGet(uint8_t, boolean);
		
		/**
		 * clockSet: Set the clock time using integer values
		 * Does not do any GMT or DST correction
		**/
	#ifdef DS1337_USE_BCD_CLOCKSET
		void		clockSet(uint8_t, uint16_t);
	#endif
		
		/**
		 * calculateUTS: returns the time as a unix time stamp
		 * This function doesn't take into account having DST set or not!
		 *
		 * Use the macro getUTS macro to access this function! 
		**/
	#ifdef DS1337_USE_GET_UTS
		uint32_t calculateUTS(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec);
	#endif
		
		/**
		 * clockSetWithUTS: sets the date & time from a unix time stamp
		 * pass the second param as true to skip DTS and GMT calculation
		 *
		 * Use the clockSet macro to access this function!
		**/
		void		clockSetWithUTS(uint32_t, boolean);
	#ifdef DS1337_DEBUG
		/**
		 * Prints all of the DS1337 registers
		**/
		void		printRegisters(void);
	#endif
	#ifdef DS1337_USE_ALARMS
		/**
		 * alarmSelect: allows selection of the DS1337 alarm 1 or 2
		 * false for alarm 1 and true for alarm 2
		**/
		void		alarmSelect(boolean);
		
		/**
		 * Allows setting an alarm's date and time using integers
		**/
		void		alarmSet(uint8_t, uint8_t);
		
		/**
		 * alarmCheck: checks if an alarm flag was set and reset the flag
		 * set param to true to check for both registers false (or void) to check for the selected alarm
		**/
		boolean		alarmCheck(boolean);
		boolean		alarmCheck(void); // Same as above using false
	
	#ifdef DS1337_USE_ALARMS_CALLBACK
		/**
		 * alarmSetCallback: allows setting of a callback function associated with alarm
		 * The function will be passed a boolean indicating which of alarm1 (false) or alarm2 (true)
		 * triggered the callback
		**/
		void		alarmSetCallback(void (*)(void));
		
		/**
		 * alarmUnsetCallback: removes the callback function attached to the current alarm
		**/
		void		alarmUnsetCallback(void);
		
		/**
		 * alarmChecks: will trigger the callback function if an alarm is high
		 * This function need to be placed somewhere in the main loop
		**/
		void		alarmChecks(void);
	#endif
	
	#ifdef DS1337_USE_ALARM_INTERRUPTS
		/**
		 * alarmDisableInterrupts: disables all alarm interrupts
		**/
		void		alarmDisableInterrupts(void);
		
		/**
		 * alarmSetInterrupt: sets the alarm interrupt for the selected alarm
		**/
		void		alarmSetInterrupt(void);
		
		/**
		 * alarmUnsetInterrupt: disable interrupt for the select alarm
		**/
		void		alarmUnsetInterrupt(void);
	#endif
	#endif
	
	#ifdef DS1337_USE_SQW_OUTPUT
		/**
		 * sqwEnable: Enable the square wave output on SQW/INTB
		 * If this is enabled and an interrupt is set for alarm 2 
		 * the interrupt will INTA instead of SQW/INTB
		**/
		void		sqwEnable(void);
		
		/**
		 * sqwDisable: Disables the square wave output on SQW/INTB
		**/
		void		sqwDisable(void);
		
		/**
		 * sqwSetRate: Sets the square wave rate
		**/
		void		sqwSetRate(uint8_t sqwRate);
	#endif
	private:
		/**
		 * Hold a unix time stamp
		**/
	#if defined(DS1337_USE_SET_UTS) || defined(DS1337_USE_GET_UTS)
		uint32_t	tt;
	#endif
		
		/**
		 * Holds the RTC BCD time buffer
		**/
		uint8_t		rtc_bcd[8];
		
		/**
		 * Writes a value to a clock register
		**/
		void		writeRegister(uint8_t, uint8_t);
		
		/**
		 * Write the RTC BCD buffer to the clock
		**/
		void		clockSave(void);
		
		/**
		 * Converts a BCD to a binary integer
		**/
		uint8_t		bcdToBin(uint8_t, uint8_t);
		
		/**
		 * Converts a binary integer to BCD
		**/
		uint8_t		binToBcd(uint8_t);
		
	#ifdef DS1337_USE_ALARMS	
		/**
		 * Hold the buffer for alarm manipulation
		**/
		uint8_t		rtc_alarm[4];
		/**
		 * alarmId: keeps track of which alarm we are working with
		**/
		boolean		alarmId;
		
		/**
		 * Writes the alarm buffer to the RTC
		**/
		void		alarmSave(void);
	#endif
};

/**
 * Define the Object's name
**/
extern DS1337 RTC;

#endif
