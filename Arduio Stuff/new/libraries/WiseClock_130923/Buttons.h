// Buttons.h

#ifndef _BUTTONS_H_
#define _BUTTONS_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "UserConf.h"

#define BUTTON_MENU_PIN	0	// "Menu" button on D0 (PB0);
#define BUTTON_SET_PIN	1	// "Set"  button on D1 (PB1);
//------------------------------------------------------------------
// revision 4.0 requires h/w intervention;
// button Plus was on D2 and SQW/INT from DS3231 was on D3;
// they need to be swapped;
//------------------------------------------------------------------
#if _WISE_CLOCK_VER > 3
#define BUTTON_PLUS_PIN	3	// "Plus" button on D3 (PB3);
#else
#define BUTTON_PLUS_PIN	2	// "Plus" button on D2 (PB2);
#endif




// last time the respective button was pressed; used for debouncing;
extern volatile unsigned long timeBtnMenu;
extern volatile unsigned long timeBtnSet;
extern volatile unsigned long timeBtnPlus;


void setupButtons();
void checkButtons(unsigned long ms);
boolean readButtons();



#endif  // _BUTTONS_H_


