/***********************************************************************
 * Sketch for the BookClock, a clock with 8x32 LED display from Sure Electronics.
 *
 * Parts of the sketch written by Bill Westfield ("WestfW")
 *   as demo for the HT1632 display.
 * Other parts written by FlorinC.
 *   Copyrighted and distributed under the terms of the Berkely license
 *   (copy freely, but include this notice of original author.)
 *
 * - clock has tilt sensor on D14 (A0);
 * - display connected on pins D6, D7 and D8;
 *
 ***********************************************************************/

// comment out this line for the 8x32 display;
//#define _16x24_

// comment this line out if you want to minimize program size;
#define _DEBUG_ true


#include <WProgram.h>
#include "ht1632.h"
#include <avr/pgmspace.h>  // fonts stored in progmem;
#include "font3.h"
#include "symbols.h"
#include <Wire.h>   
#include <DS1307.h>   


// when 1 (D14 pulled up to Vcc), BookClock is standing;
#define PIN_TILT_SENSOR  14

// output pins for display;
#define HT1632_DATA     6    // Data pin (pin 7)
#define HT1632_WRCLK    7    // Write clock pin (pin 5)
#define HT1632_CS       8    // Chip Select (1, 2, 3, or 4)



// I2C Bus address of 24LC256 256K EEPROM;   
// if more than one eeprom, they will have different addresses (h/w configured);   
#define I2C_ID     0x50   
  
// global address of the last byte read from eeprom;   
// set to a random value in setup();   
unsigned int crtReadAddress = 0;   



// when alternating between displaying quotes and time, start with the time;   
boolean isDisplayingTime  = true;   
boolean isDisplayingQuote = false;   

int hour, minute, second, day, month, year;


#ifdef _16x24_
  #define X_MAX 23
  #define Y_MAX 15
#else
  #define X_MAX 31
  #define Y_MAX 7
#endif


#define plot(x,y,v)  ht1632_plot(x,y,v)
#define cls          ht1632_clear

#define DISPDELAY 0



// current line string is built by copying each character to the position of the pointer, then advancing the pointer;
char  msgLine[180] = {0};
char* msgLinePtr = &msgLine[0];
int   msgLineSize  = 0;
int   msgLineIndex = 0;


// use this buffer to display the time (seconds are ignored);
char* timeBuffer = "      12:45";
int   timeBufferIndex = 0;
int   timeBufferSize  = strlen(timeBuffer);


char hourSprite[8];
char minSprite[8];
char monthSprite[8];
char daySprite[8];



/***********************************************************************
 * ht1632_chipselect / ht1632_chipfree
 * Select or de-select a particular ht1632 chip.
 * De-selecting a chip ends the commands being sent to a chip.
 * CD pins are active-low; writing 0 to the pin selects the chip.
 ***********************************************************************/

void ht1632_chipselect(byte chipno)
{
  digitalWrite(chipno, 0);
}


void ht1632_chipfree(byte chipno)
{
  digitalWrite(chipno, 1);
}


/*
 * we keep a copy of the display controller contents so that we can
 * know which bits are on without having to (slowly) read the device.
 * Note that we only use the low four bits of the shadow ram, since
 * we're shadowing 4-bit memory.  This makes things faster, and we
 * use the other half for a "snapshot" when we want to plot new data
 * based on older data...
 */
// (fc) covers the case for both 16x24 (96 bytes, 4 bits) and 32x8 (64 bytes, 4 bits);
byte ht1632_shadowram[96];  // our copy of the display's RAM


/*
 * ht1632_writebits
 * Chip is assumed to already be chip-selected
 * Bits are shifted out from MSB to LSB, with the first bit sent
 * being (bits & firstbit), shifted till firsbit is zero.
 */
void ht1632_writebits (byte bits, byte firstbit)
{
  while (firstbit)
  {
    digitalWrite(HT1632_WRCLK, LOW);
    if (bits & firstbit)
    {
      digitalWrite(HT1632_DATA, HIGH);
    }
    else
    {
      digitalWrite(HT1632_DATA, LOW);
    }
    digitalWrite(HT1632_WRCLK, HIGH);
    firstbit >>= 1;
  }
}


/*
 * ht1632_sendcmd
 * Send a command to the ht1632 chip.
 * A command consists of a 3-bit "CMD" ID, an 8bit command, and
 * one "don't care bit".
 *   Select 1 0 0 c7 c6 c5 c4 c3 c2 c1 c0 xx Free
 */
static void ht1632_sendcmd (byte command)
{
  ht1632_chipselect(HT1632_CS);          // Select chip
  ht1632_writebits(HT1632_ID_CMD, 1<<2); // send 3 bits of id: COMMMAND
  ht1632_writebits(command, 1<<7);       // send the actual command
  ht1632_writebits(0, 1);                // one extra dont-care bit in commands
  ht1632_chipfree(HT1632_CS);            // done
}


/*
 * ht1632_clear
 * clear the display, and the shadow memory, and the snapshot
 * memory.  This uses the "write multiple words" capability of
 * the chipset by writing all 96 words of memory without raising
 * the chipselect signal.
 */
void ht1632_clear()
{
  char i;

  ht1632_chipselect(HT1632_CS);          // Select chip
  ht1632_writebits(HT1632_ID_WR, 1<<2);  // send ID: WRITE to RAM
  ht1632_writebits(0, 1<<6);             // Send address
  for (i = 0; i < 96/2; i++)             // Clear entire display
    ht1632_writebits(0, 1<<7);           // send 8 bits of data
  ht1632_chipfree(HT1632_CS);            // done

  for (i=0; i<96; i++)
    ht1632_shadowram[i] = 0;
}


/*
 * ht1632_senddata
 * send a nibble (4 bits) of data to a particular memory location of the
 * ht1632.  The command has 3 bit ID, 7 bits of address, and 4 bits of data.
 *    Select 1 0 1 A6 A5 A4 A3 A2 A1 A0 D0 D1 D2 D3 Free
 * Note that the address is sent MSB first, while the data is sent LSB first!
 * This means that somewhere a bit reversal will have to be done to get
 * zero-based addressing of words and dots within words.
 */
static void ht1632_senddata (byte address, byte data)
{
  ht1632_chipselect(HT1632_CS);          // Select chip
  ht1632_writebits(HT1632_ID_WR, 1<<2);  // send ID: WRITE to RAM
  ht1632_writebits(address, 1<<6);       // Send address
  ht1632_writebits(data, 1<<3);          // send 4 bits of data
  ht1632_chipfree(HT1632_CS);            // done
}


void ht1632_setup()
{
  pinMode(HT1632_CS, OUTPUT);
  digitalWrite(HT1632_CS, HIGH);      // unselect (active low)
  pinMode(HT1632_WRCLK, OUTPUT);
  pinMode(HT1632_DATA, OUTPUT);
  ht1632_sendcmd(HT1632_CMD_SYSDIS);  // Disable system

#ifdef _16x24_
  ht1632_sendcmd(HT1632_CMD_COMS11);  // 16*32, PMOS drivers
#else
// (fc)
  ht1632_sendcmd(HT1632_CMD_COMS10);  // 32x8, PMOS drivers
#endif

  ht1632_sendcmd(HT1632_CMD_MSTMD); 	// Master Mode
  ht1632_sendcmd(HT1632_CMD_SYSON); 	// System on
  ht1632_sendcmd(HT1632_CMD_LEDON); 	// LEDs on

  for (byte i=0; i<96; i++)
    ht1632_senddata(i, 0);  // clear the display!

  delay(100);  // REM:
}


/*
 * Copy a character glyph from the myfont data structure to
 * display memory, with its upper left at the given coordinate
 * This is unoptimized and simply uses plot() to draw each dot.
 */
void ht1632_putchar(int x, int y, char c)
{
  // fonts defined for ascii 32 and beyond (index 0 in font array is ascii 32);
  byte charIndex;

  // replace undisplayable characters with blank;
  if (c < 32 || c > 126)
  {
    charIndex = 0;
  }
  else
  {
    charIndex = c - 32;
  }

  // move character definition, pixel by pixel, onto the display;
  // fonts are defined as one byte per row;
  for (byte row=0; row<8; row++)
  {
    byte rowDots = pgm_read_byte_near(&myfont[charIndex][row]);
    for (byte col=0; col<6; col++)
    {
      if (rowDots & (1<<(5-col)))
        plot(x+col, y+row, 1);
      else
        plot(x+col, y+row, 0);
    }
  }
}


/*
 * plot a point on the display, with the upper left hand corner
 * being (0,0), and the lower right hand corner being (23, 15).
 * Note that Y increases going "downward" in contrast with most
 * mathematical coordiate systems, but in common with many displays
 * No error checking; bad things may happen if arguments are out of
 * bounds!  (The ASSERTS compile to nothing by default
 */
void ht1632_plot(int x, int y, char val)
{
  if (x<0 || x>X_MAX || y<0 || y>Y_MAX)
     return;

  char addr, bitval;

  /*
   * The 4 bits in a single memory word go DOWN, with the LSB
   * (first transmitted) bit being on top.  However, writebits()
   * sends the MSB first, so we have to do a sort of bit-reversal
   * somewhere.  Here, this is done by shifting the single bit in
   * the opposite direction from what you might expect.
   */
  bitval = 8>>(y&3);       // compute which bit will need set

  // compute which memory byte this is in;
#ifdef _16x24_
  addr = (x<<2) + (y>>2);
#else
// (fc)
  addr = (x<<1) + (y>>2);
#endif

  if (val) {  // Modify the shadow memory
    ht1632_shadowram[addr] |= bitval;
  }
  else {
    ht1632_shadowram[addr] &= ~bitval;
  }

  // Now copy the new memory value to the display
  ht1632_senddata(addr, ht1632_shadowram[addr]);
}


/*
 * get_shadowram
 * return the value of a pixel from the shadow ram.
 */
byte get_shadowram(byte x, byte y)
{
  byte addr, bitval;

  bitval = 8>>(y&3);       // compute which bit will need set
  addr = (x<<2) + (y>>2);  // compute which memory word this is in

  return (0 != (ht1632_shadowram[addr] & bitval));
}


/*
 * snapshot_shadowram
 * Copy the shadow ram into the snapshot ram (the upper bits)
 * This gives us a separate copy so we can plot new data while
 * still having a copy of the old data.  snapshotram is NOT
 * updated by the plot functions (except "clear")
 */
void snapshot_shadowram()
{
  for (char i=0; i< sizeof(ht1632_shadowram); i++)
  {
    ht1632_shadowram[i] = (ht1632_shadowram[i] & 0x0F) | ht1632_shadowram[i] << 4;  // Use the upper bits
  }
}


/*
 * get_snapshotram
 * get a pixel value from the snapshot ram (instead of
 * the actual displayed (shadow) memory
 */
byte get_snapshotram(byte x, byte y)
{
  byte addr, bitval;

  bitval = 128>>(y&3);  // user upper bits!

  // compute which memory byte this is in;
#ifdef _16x24_
  addr = (x<<2) + (y>>2);
#else
// (fc)
  addr = (x<<1) + (y>>2);
#endif

  if (ht1632_shadowram[addr] & bitval)
    return 1;

  return 0;
}



/*
* This works equally well for both 16x24 and 8x32 matrices.
*/
void displayScrollingLine(char* msg, int crtPos)
{
  // shift the whole screen 6 times, one column at a time;
  for (int x=0; x<6; x++)
  {
    ht1632_putchar(-x, 0, msg[crtPos]);
    ht1632_putchar(-x+6,  0, ((crtPos+1 < strlen(msg)) ? msg[crtPos+1] : ' '));
    ht1632_putchar(-x+12, 0, ((crtPos+2 < strlen(msg)) ? msg[crtPos+2] : ' '));
    ht1632_putchar(-x+18, 0, ((crtPos+3 < strlen(msg)) ? msg[crtPos+3] : ' '));
    ht1632_putchar(-x+24, 0, ((crtPos+4 < strlen(msg)) ? msg[crtPos+4] : ' '));
    ht1632_putchar(-x+30, 0, ((crtPos+5 < strlen(msg)) ? msg[crtPos+5] : ' '));
    ht1632_putchar(-x+36, 0, ((crtPos+6 < strlen(msg)) ? msg[crtPos+6] : ' '));
    delay(DISPDELAY);
  }
}


void displayScrollingLine()
{
  // shift the whole screen 6 times, one column at a time;
  for (int x=0; x<6; x++)
  {
    ht1632_putchar(-x, 0, msgLine[msgLineIndex]);
    ht1632_putchar(-x+6,  0, ((msgLineIndex+1 < strlen(msgLine)) ? msgLine[msgLineIndex+1] : ' '));
    ht1632_putchar(-x+12, 0, ((msgLineIndex+2 < strlen(msgLine)) ? msgLine[msgLineIndex+2] : ' '));
    ht1632_putchar(-x+18, 0, ((msgLineIndex+3 < strlen(msgLine)) ? msgLine[msgLineIndex+3] : ' '));
    ht1632_putchar(-x+24, 0, ((msgLineIndex+4 < strlen(msgLine)) ? msgLine[msgLineIndex+4] : ' '));
    ht1632_putchar(-x+30, 0, ((msgLineIndex+5 < strlen(msgLine)) ? msgLine[msgLineIndex+5] : ' '));
    ht1632_putchar(-x+36, 0, ((msgLineIndex+6 < strlen(msgLine)) ? msgLine[msgLineIndex+6] : ' '));
    delay(DISPDELAY);
  }
/*
  msgLineIndex++;
  if (msgLineIndex >= strlen(msgLine))
  {
    msgLineIndex = 0;
  }
*/
}


void readTimeFromRTC()  
{  
  int rtc[7];  
  RTC.get(rtc, true);  
 
  // update global variables from RTC;
  second = rtc[0];  
  minute = rtc[1];  
  hour   = rtc[2];  
  day    = rtc[4];  
  month  = rtc[5];  
  year   = rtc[6];  
 
#ifdef _DEBUG_
    Serial.print("Time is ");  
    Serial.print(hour);  
    Serial.print(":");  
    Serial.print(minute);  
    Serial.println("");  
#endif
 
  // update the string containing formatted time (for horizontal display);
  timeBuffer[6]  = (hour < 10) ? ' ' : ('0' + hour/10);  
  timeBuffer[7]  = '0' + hour%10;  
  timeBuffer[8]  = ':';  
  timeBuffer[9] = '0' + minute/10;  
  timeBuffer[10]= '0' + minute%10;  

  // update the sprites containing time (for vertical display);
  createTwoDigitSprite(hourSprite, hour, false);
  createTwoDigitSprite(minSprite, minute, true);
  createTwoDigitSprite(daySprite, day, false);
}


byte readByte(int i2cId, unsigned int eeaddress)  
{  
  byte rdata = 0xFF;  
  Wire.beginTransmission(i2cId);  
  Wire.send((int)(eeaddress >> 8));    // Address High Byte  
  Wire.send((int)(eeaddress & 0xFF));  // Address Low Byte  
  Wire.endTransmission();  
  Wire.requestFrom(i2cId, 1);  
  if (Wire.available()) rdata = Wire.receive();  
  return rdata;  
}  


byte readNextByte()  
{  
  byte rdata = readByte(I2C_ID, crtReadAddress);  
  crtReadAddress++;  
  return rdata;  
}  



// read, from I2C EEPROM, a whole line (ending with CR) into the message buffer;  
void fetchLineFromEprom()  
{  
  byte lastReadByte = readNextByte();  
 
  // after reaching the end of eprom's content, start from the beginning;   
  if (lastReadByte == 0xFF || lastReadByte == 0)   
  {   
    crtReadAddress = 0;   
    lastReadByte   = readNextByte();   
  }   
  
  // insert a few blanks at the beginning;
  *msgLinePtr++ = ' ';   
  *msgLinePtr++ = ' ';   
  *msgLinePtr++ = ' ';   
  *msgLinePtr++ = ' ';   
  *msgLinePtr++ = ' ';   
  *msgLinePtr++ = ' ';   
  
  while (lastReadByte != 13 && lastReadByte != 0xFF && lastReadByte != 0)   
  {   
    *msgLinePtr++ = lastReadByte;   
    lastReadByte  = readNextByte();   
  }   
  
  // insert a few blanks at the end;
  *msgLinePtr++ = ' ';   
  *msgLinePtr++ = ' ';   

  // mark the end of the string;   
  *msgLinePtr++ = 0;   
  
#ifdef _DEBUG_
  Serial.println(msgLine);
#endif

  msgLinePtr  = &msgLine[0];      // reset the string pointer;   
  msgLineSize = strlen(msgLine);  // update the size of the current string;   
}   


void displayingQuote()   
{   
  displayScrollingLine(msgLine, msgLineIndex);
  
  msgLineIndex++;   
  if (msgLineIndex >= msgLineSize)   
  {   
    fetchLineFromEprom();   
    msgLineIndex = 0;
  }   
}   


// show time formatted HH:MM that fits on 32 columns;
// REM: other time formats (that include date) need scrolling;
void displayTime()   
{
    ht1632_putchar( 1, 0, timeBuffer[6]);
    ht1632_putchar( 7, 0, timeBuffer[7]);
    ht1632_putchar(13, 0, timeBuffer[8]);
    ht1632_putchar(19, 0, timeBuffer[9]);
    ht1632_putchar(25, 0, timeBuffer[10]);
}   


void displayHorizontal()   
{
  displayScrollingLine(msgLine, msgLineIndex);
  
  msgLineIndex++;   
  if (msgLineIndex >= msgLineSize)   
  {   
    // the end of a quotation was reached;
    // reload quotation...
    fetchLineFromEprom();
    msgLineIndex = 0;

    // ... and display the time;
    readTimeFromRTC();
    displayTime();
    delay(3000);
  }
}


void displayVertical()
{
  readTimeFromRTC();
  ht1632_putSprite( 0, 0, hourSprite);
  ht1632_putSprite( 8, 0, minSprite);
  ht1632_putSprite(16, 0, monthSymbol[month-1]);
  ht1632_putSprite(24, 0, daySprite);
  delay(3000);
}


void createTwoDigitSprite(char* destSprite, int number, boolean leadingZero)
{
  int h1 = number / 10;
  int h2 = number % 10;

  for (byte i = 0; i < 8; i++)
  {
    char row = 0;
    if (i>3)
    {
      row = (leadingZero ? digit[h1][i-4] : (h1 ? digit[h1][i-4] : 0));
    }
    else
    {
      row = digit[h2][i];
    }
    
    destSprite[i] = row;
  }
}


void ht1632_putSprite(int x, int y, char* sprite)
{
  // move sprite, pixel by pixel, onto the display;
  for (byte row=0; row<8; row++)
  {
    byte rowDots = sprite[row];
    for (byte col=0; col<8; col++)
    {
      if (rowDots & (1<<(7-col)))
        plot(x+col, y+row, 1);
      else
        plot(x+col, y+row, 0);
    }
  }
}



void setup ()
{
  ht1632_setup();

#ifdef _DEBUG_
  Serial.begin(9600);
#endif

  pinMode(PIN_TILT_SENSOR, INPUT);

  cls();
}


void loop()
{
  int tilt = digitalRead(PIN_TILT_SENSOR);
#ifdef _DEBUG_
  Serial.print("Tilt sensor value=");
  Serial.println(tilt, DEC);
#endif

//  tilt = HIGH;
  
  if (tilt == HIGH)
  {
    // BookClock is standing;
    // in each 8x8 cell of the display, show hour, minute, month and day;
    displayVertical();
  }
  else
  {
    // BookClock is sitting;
    // display scrolling quotations retrieved from eeprom;
    // display time between quotations;
    displayHorizontal();
  }
}


