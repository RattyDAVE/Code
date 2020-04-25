/*
*********************************************************************************
* Watch.pde	- display time as an analog watch, by turning on 2 LEDs:
*             1 of 12 for hour, another 1 of 12 for minute;
*		- uses the 3V Wiseduino, with DS1337 RTC;
*		- ATmega168 is mostly in sleep mode, except when the
*		  "display" button is pushed, which reads time from RTC
*		  and lights up the hour and minute LEDs;
*
*********************************************************************************
*/

#include <WProgram.h>
#include <avr/sleep.h>
#include <Wire.h>
#include <DS1337.h>


#define _DEBUG_


// push button on D3; used for waking up controller;
#define PIN_BUTTON   3


// The display is made of 24 LEDs positioned in a 5x5 matrix.
// columns: D4, D5, D6, D7, D8
// rows: D9, D14/A0, D15/A1, D16/A2, D17/A3
int rows[] = {9, 14, 15, 16, 17};
int cols[] = {4, 5, 6, 7, 8};


boolean isShowingTime = false;
boolean isSettingUpTime = false;
int hourSet, minSet;
long lastBtnPress;

int nBlinkMin = 0;
long blinkCounterMin = 0;
long blinkCounterHour = 0;
boolean isHourBlinking = true;
boolean isMinBlinking  = true;


struct pos
{
  public:
    int row;
    int col;
    pos(int r, int c) {row=r; col=c;}
};


// hours are shown by lighting the following pairs (row, col):
pos posMin[12] = {pos(0, 2), pos(0, 4), pos(1, 4), pos(2, 4), pos(3, 4), pos(4, 4), pos(4, 2), pos(4, 0), pos(3, 0), pos(2, 0), pos(1, 0), pos(0, 0)};

// minutes are shown by lighting the following pairs (row, col):
pos posHour[12] = {pos(1, 2), pos(0, 3), pos(1, 3), pos(2, 3), pos(3, 3), pos(4, 3), pos(3, 2), pos(4, 1), pos(3, 1), pos(2, 1), pos(1, 1), pos(0, 1)};


byte soft_prescaler = 0;


//********************************************************************
void setup()
{
  // setup display;
  for (int i=0; i<5; i++)
  {
    pinMode(rows[i], OUTPUT);
    pinMode(cols[i], OUTPUT);
  }

  clearDisplay();

  pinMode(PIN_BUTTON, INPUT);

  // setup RTC;
  RTC.Init();	// essential call;
  RTC.clockStart();

#ifdef _DEBUG_
  Serial.begin(9600);
#endif

  // define ISR for pin D3;
  attachInterrupt(1, buttonISR, LOW);

  // go directly to sleep and wait for the button to be pushed;
  sleepNow();
}


//********************************************************************
void loop()
{
  if (isSettingUpTime)
  {
    if (digitalRead(PIN_BUTTON) == LOW)
    {
      lastBtnPress = millis();
      
      minSet++;
      if (minSet >= 60)
      {
        minSet = 0;
        hourSet++;
        hourSet = hourSet % 12;
      }
      
      ShowSimpleTime(hourSet, minSet);
      delay(40);
    }
    else
    {
      ShowSimpleTime(hourSet, minSet);

      if (millis() - lastBtnPress > 3000)
      {
        // save the time;
        Serial.println("Saving time");

        isSettingUpTime = false;

        attachInterrupt(1, buttonISR, LOW);
      
        isShowingTime = false;
//      clearDisplay();

        // go back to sleep;
        sleepNow();
      }
    }
  }
  else if (isShowingTime)
  {
    detachInterrupt(1);

    // read time from RTC;
    int hour   = RTC.clockGet(DS1337_HR, true);
    int minute = RTC.clockGet(DS1337_MIN, true);
    int second = RTC.clockGet(DS1337_SEC, true);
  
#ifdef _DEBUG_
    Serial.print("Time is: ");
    Serial.print(hour);
    Serial.print(":");
    Serial.print(minute);
    Serial.print(":");
    Serial.println(second);
#endif

#ifdef _DEBUG_
//    testHours();
//    testMinutes();
#endif

    // hours is between 0 and 23; must be mapped to 0..11;
//    int dHour = (minute > 30 ? hour+1 : hour) % 12;
    int hour1 = hour % 12;
    int hour2 = (minute > 30 ? hour+1 : hour) % 12;

    // minute is between 0 and 59; must be mapped to 0..11;
    int min1 = minute/5;
    int min2 = (min1+1) % 12;
    int min1Blinks = minute % 5;
    int min2Blinks = 5 - min1Blinks;

    int rowH  = posHour[hour1].row;
    int colH  = posHour[hour1].col;
    int rowH2 = posHour[hour2].row;
    int colH2 = posHour[hour2].col;
    int rowM  = posMin[min1].row;
    int colM  = posMin[min1].col;
    int rowM2 = posMin[min2].row;
    int colM2 = posMin[min2].col;

    long t1 = millis();
    while (millis()-t1 < 4000)
    {
      if (hour1 != hour2)
      {
        // will blink 2 LEDs alternatively for hour;
        blinkCounterHour++;
        if (blinkCounterHour>5)
        {
          blinkCounterHour = 0;
          isHourBlinking = !isHourBlinking;
        }

        if (isHourBlinking)
        {
          // light up the hour LED;
          flickerLED(colH, rowH);
        }
        else
        {
          // light up the other hour LED;
          flickerLED(colH2, rowH2);
        }
      }
      else
      {
        // "solid" hour (no blinking);
        flickerLED(colH, rowH);
      }

      if (min1Blinks == 0)
      {
        // "solid" minute;
        flickerLED(colM, rowM);
      }
      else
      {
        if (isMinBlinking)
        {
          // total of 5 blinks (for both LEDs);
          if (nBlinkMin < 5 - min1Blinks)
          {
            // light up the minute LED for a number of times;
            flickerLED(colM, rowM);
          }
          else if (nBlinkMin < 5)
          {
            // light the other minute for a number of times;
            flickerLED(colM2, rowM2);
          }
        }

        blinkCounterMin++;
        if (blinkCounterMin>45)
        {
          blinkCounterMin = 0;
          isMinBlinking = !isMinBlinking;

          if (isMinBlinking)
              nBlinkMin++;
        }
      }
    }
    
    // check the button; user may want to set up time;
    if (digitalRead(PIN_BUTTON) == LOW)
    {
      if (!isSettingUpTime)
      {
        Serial.println("setting up the clock...");
        isSettingUpTime = true;
//        clearDisplay();
        hourSet = hour;
        minSet = minute;
      }
    }
    else
    {
      attachInterrupt(1, buttonISR, LOW);
      
      isShowingTime = false;
//      clearDisplay();

      // go back to sleep;
      sleepNow();
    }
  }
}


//------------------------------------------------------------------------
// put the arduino to sleep to save power;
// function copied from http://www.arduino.cc/playground/Learning/arduinoSleepCode;
//------------------------------------------------------------------------
void sleepNow()
{
    /* Now is the time to set the sleep mode. In the Atmega8 datasheet
     * http://www.atmel.com/dyn/resources/prod_documents/doc2486.pdf on page 35
     * there is a list of sleep modes which explains which clocks and 
     * wake up sources are available in which sleep modus.
     *
     * In the avr/sleep.h file, the call names of these sleep modus are to be found:
     *
     * The 5 different modes are:
     *     SLEEP_MODE_IDLE         -the least power savings 
     *     SLEEP_MODE_ADC
     *     SLEEP_MODE_PWR_SAVE
     *     SLEEP_MODE_STANDBY
     *     SLEEP_MODE_PWR_DOWN     -the most power savings
     *
     * For now, we want as much power savings as possible, so we 
     * choose the according 
     * sleep modus: SLEEP_MODE_PWR_DOWN
     * 
     */  
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here

    sleep_enable();          // enables the sleep bit in the mcucr register so sleep is possible; just a safety pin;

    detachInterrupt(1);      // disables the previous routine (button ISR);
    attachInterrupt(1, wakeUpNow, LOW); // use interrupt 1 (pin D3) and run function wakeUpNow when pin D3 gets LOW (button push);

#ifdef _DEBUG_
    Serial.println("going to sleep...");
#endif

    sleep_mode();        // the device is actually put to sleep! THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP.

#ifdef _DEBUG_
    Serial.println("I am back!");
#endif

    sleep_disable();         // first thing after waking from sleep: disable sleep...

    detachInterrupt(1);      // disables interrupt 1 (D3) so the wakeUpNow code will not be executed during normal running time;
    attachInterrupt(1, buttonISR, LOW);
}


void wakeUpNow()        // here the interrupt is handled after wakeup
{
  // execute code here after wake-up before returning to the loop() function
  // timers and code using timers (serial.print and more...) will not work here.
  // we don't really need to execute any special functions here, since we just want the thing to wake up;
}


void setRtcTime()
{
/*
  // set the clock using a unix time stamp (date -u +'%s')...
  uint32_t unixTime = RTC.calculateUTS(2010, 4, 17, 12, 25, 32);
  RTC.clockSetWithUTS(unixTime, false);
*/

  // ... or set each part individually
  RTC.clockSet(DS1337_HR, 12);
  RTC.clockSet(DS1337_HR, 11);
  RTC.clockSet(DS1337_HR, 32);
}


// called as a result of the button being pressed;
// triggers the activation of the display (shows time);
void buttonISR()
{
  isShowingTime = true;

  nBlinkMin = 0;
  blinkCounterMin  = 0;
  blinkCounterHour = 0;

  isHourBlinking = true;
  isMinBlinking  = true;

  isSettingUpTime = false;
}


void clearDisplay()
{
  for (int i=0; i<5; i++)
    digitalWrite(cols[i], LOW);

  for (int j=0; j<5; j++)
    digitalWrite(rows[j], HIGH);
}


#ifdef _DEBUG_
void testHours()
{
  for (int i=0; i<12; i++)
  {
    clearDisplay();

    int row = posHour[i].row;
    int col = posHour[i].col;

    // light up the hour LED;
    digitalWrite(cols[col], HIGH);
    digitalWrite(rows[row], LOW);
    delay(500);
  }
}
#endif


#ifdef _DEBUG_
void testMinutes()
{
  for (int i=0; i<12; i++)
  {
    clearDisplay();

    int row = posMin[i].row;
    int col = posMin[i].col;

    // light up the minute LED;
    digitalWrite(cols[col], HIGH);
    digitalWrite(rows[row], LOW);
    delay(500);
  }
}
#endif


void flickerLED(int col, int row)
{
      // light up the hour LED;
      digitalWrite(cols[col], HIGH);
      digitalWrite(rows[row], LOW);
      delay(5);
      digitalWrite(cols[col], LOW);
      digitalWrite(rows[row], HIGH);
}


void ShowSimpleTime(int hourSet, int minSet)
{
  Serial.print("show time ");
  Serial.print(hourSet);
  Serial.print(":");
  Serial.println(minSet);
 
  int rowH = posHour[hourSet].row;
  int colH = posHour[hourSet].col;
  int rowM = posMin[minSet/5].row;
  int colM = posMin[minSet/5].col;

  flickerLED(colH, rowH);
  flickerLED(colM, rowM);
}




