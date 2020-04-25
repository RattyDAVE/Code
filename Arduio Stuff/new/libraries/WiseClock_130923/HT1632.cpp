/*
 *********************************************************************************************************
 * HT1632.cpp
 *
 * Apr/10 by FlorinC (http://timewitharduino.blogspot.com/)
 *   Copyrighted and distributed under the terms of the Berkeley license
 *   (copy freely, but include this notice of original authors.)
 *
 * Adapted after HT1632 library by Bill Westfield ("WestfW") (http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1225239439/0);
 *
 * Other contributors:
 *   - fWrite (fast writes) functions courtesy of BroHogan (http://brohogan.blogspot.com/);
 *   - Mark Sproul <MLS> msproul _at_ jove.rutgers.edu
 *
 *********************************************************************************************************
 */

//*********************************************************************************************************
//*	Edit History, started April, 2010
//*	please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//* Apr 15/10 (fc) created file, by restructuring Wise4Sure.pde;
//* Oct 10/10 (rp) ht1632_putBigDigit() amended to allow for multiple fonts
//* Jan 29/11 (fc) adapted to 3216 bi-color display;
//* Jun 12/11 (rp) ht1632_putchar(), ht1632_putSmallChar() amended for ascii character 127 (degree symbol)
//*	Jun 12/11 (rp) ht1632_putBigDigit() now has color and columns parameter;
//* Jun 20/11 (fc) added ht1632_putBitmap(), ht1632_putTinyChar(), setBrightness();
//* Oct 15/11 (rp) added overlayWithSnapshotHorizontal(), overlayWithSnapshotVertical(), ht1632_putLargeChar();
//*					added PUTINSNAPSHOTRAM option plus several small speed optimalizations;
//* Mar 25/12 (rp) added ht1632_copyToVideo() for animations, ht1632_clear() corrected (too many zeroes, 92 -> 64);
//*                To speed up the display ht1632_plot() and ht1632_putLargeChar() now expects bytes instead of integers;
//* Apr 24/12 (mcm) parameterized for multiple displays
//* Apr 24/12 (mcm) small optimizations to ht1632_clear(), ht1632_plot(), put_snapshotram()
//* Apr 25/12 (rp) added ht1632_copyToVideo(chipNo, Buffer);
//* Apr 26/12 (mcm) changed shadowram[64][4] to shadowram[4][64] to better reflect hardware layout
//*		    saved ~200 bytes of program space!
//* May 05/12 (mcm) use int8_t or int16_t instead of int where appropriate
//* May 20/12 (mcm) #define RESTRICT_SCROLL_DISPLAY to limit scrolls to only one display
//*		    used when day/night graph is on 2nd display, scroll message on 1st display
//* May 28/12 (mcm) restructured to eliminate as many delay() calls as possible
//* Jun 14/12 (mcm) x coordinates now 'coord_t' type, width dependent on NUM_DISPLAYS
//* Jul 02/12 (mcm) add #define SMALL_CHAR, combine putSmallChar() and putchar()
//* Sep 08/13 (mcm) added displayStaticLine_P() to display strings in PROGMEM
//* Sep 10/13 (mcm) move common config options to UserConf.h
//*
//*********************************************************************************************************


#include "HT1632.h"
#include <avr/pgmspace.h>	// fonts are now loaded in program space;
#include "font3.h"
#include "fontBig.h"
#include "fontSmall.h"
#include "fontTiny.h"
#include "bitmaps2.h"
#include "fontLarge.h"


#define CLK_DELAY


#define HT1632_DATA	14	// Data pin (pin 7 of display connector)
#define HT1632_CS	13	//  Chip Select (pin 1 of display connnector)
#define HT1632_WRCLK	12	// Write clock pin (pin 5 of display connector)
#if _WISE_CLOCK_VER > 3
#define HT1632_CLK	15	// clock pin (pin 2 of display connector)
#else
#define HT1632_CLK	11	// clock pin (pin 2 of display connector)
#endif


//Atmega644/1284 Version of fastWrite - for pins 0-15
#define fWriteA(_pin_, _state_) ( _pin_ < 8 ? (_state_ ? PORTB |= 1 << _pin_ : \
PORTB &= ~(1 << _pin_ )) : (_state_ ? PORTD |= 1 << (_pin_ -8) : PORTD &= ~(1 << (_pin_ -8) )))

//Atmega644/1284 Version of fastWrite - for pins 16-31 (Note: PORTA mapping reversed from others)
#define fWriteB(_pin_, _state_) ( _pin_ < 24 ? (_state_ ? PORTC |= 1 << (_pin_ -16) : \
PORTC &= ~(1 << (_pin_ -16))) : (_state_ ? PORTA |= 1 << (31- _pin_) : PORTA &= ~(1 << (31- _pin_) )))



//*********************************************************************************************************
// our own copy of the "video" memory; 64 bytes for each of the 4 screen quarters;
// each 64-element array maps 2 planes:
// indexes from 0 to 31 are allocated for green plane;
// indexes from 32 to 63 are allocated for red plane;
// when a bit is 1 in both planes, it is displayed as orange (green + red);
byte ht1632_shadowram[CHIP_MAX][64] = {0};

//**************************************************************************************************
//Function Name: OutputCLK_Pulse
//Function Feature: enable CLK_74164 pin to output a clock pulse
//Input Argument: void
//Output Argument: void
//**************************************************************************************************
void OutputCLK_Pulse(void) //Output a clock pulse
{
	fWriteA(HT1632_CLK, HIGH);
	//  digitalWrite(HT1632_CLK, HIGH);
	fWriteA(HT1632_CLK, LOW);
	//  digitalWrite(HT1632_CLK, LOW);
}


//**************************************************************************************************
//Function Name: OutputA_74164
//Function Feature: enable pin A of 74164 to output 0 or 1
//Input Argument: x: if x=1, 74164 outputs high. If x?1, 74164 outputs low.
//Output Argument: void
//**************************************************************************************************
void OutputA_74164(unsigned char x) //Input a digital level to 74164
{
	fWriteA(HT1632_CS, (x==1 ? HIGH : LOW));
//    digitalWrite(HT1632_CS, (x==1 ? HIGH : LOW));
}


//**************************************************************************************************
//Function Name: ht1632_chipselect
//Function Feature: enable HT1632C
//Input Argument: select: HT1632C to be selected
// If select=0, select none.
// If s<0, select all.
//Output Argument: void
//**************************************************************************************************
void ht1632_chipselect(int8_t select)
{
  unsigned char tmp = 0;
  if(select<0) //Enable all HT1632Cs
  {
    OutputA_74164(0);
    CLK_DELAY;
    for(tmp=0; tmp<CHIP_MAX; tmp++)
    {
      OutputCLK_Pulse();
    }
  }
  else if(select==0) //Disable all HT1632Cs
  {
    OutputA_74164(1);
    CLK_DELAY;
    for(tmp=0; tmp<CHIP_MAX; tmp++)
    {
      OutputCLK_Pulse();
    }
  }
  else
  {
    OutputA_74164(1);
    CLK_DELAY;
    for(tmp=0; tmp<CHIP_MAX; tmp++)
    {
      OutputCLK_Pulse();
    }
    OutputA_74164(0);
    CLK_DELAY;
    OutputCLK_Pulse();
    CLK_DELAY;
    OutputA_74164(1);
    CLK_DELAY;
    tmp = 1;
    for( ; tmp<select; tmp++)
    {
      OutputCLK_Pulse();
    }
  }
}


//*********************************************************************************************************

void ht1632_setup()
{
  pinMode(HT1632_CS, OUTPUT);
  digitalWrite(HT1632_CS, HIGH); 	/* unselect (active low) */
  pinMode(HT1632_WRCLK, OUTPUT);
  pinMode(HT1632_DATA, OUTPUT);
  pinMode(HT1632_CLK, OUTPUT);

  for (int8_t j=1; j<=CHIP_MAX; j++)
  {
    ht1632_sendcmd(j, HT1632_CMD_SYSDIS);  // Disable system
    ht1632_sendcmd(j, HT1632_CMD_COMS00);
    ht1632_sendcmd(j, HT1632_CMD_MSTMD); 	/* Master Mode */
    ht1632_sendcmd(j, HT1632_CMD_RCCLK);  // HT1632C
    ht1632_sendcmd(j, HT1632_CMD_SYSON); 	/* System on */
    ht1632_sendcmd(j, HT1632_CMD_LEDON); 	/* LEDs on */
  }
 
  ht1632_clear(-1);
  delay(10);
}


//*********************************************************************************************************
/*
 * plot a point on the display, with the upper left hand corner
 * being (0,0), and the lower right hand corner being (31, 15);
 * parameter "color" could have one of the 4 values:
 * black (off), red, green or yellow;
 */
void ht1632_plot (coord_t x, int8_t y, byte color)
{
  if (x<0 || x>=X_MAX || y<0 || y>=Y_MAX)
    return;
  
  if (color & PUTINSNAPSHOTRAM)
  {
    put_snapshotram(x, y, color & ORANGE);
    return;
  }
  
  if ((color & ORANGE) != color)
    return;
  
//  int8_t chipNo = x / 16 + (y > 7 ? 2 : 0) ;
  int8_t chipNo = ((x & 0x10) >> 4) | ((y & 8) >> 2);
#if NUM_DISPLAYS > 1
  chipNo |= ((x & 0xffe0) >> 3);
#endif
  y &= 7;
  byte addr = ((x & 0xf) << 1) + (y >> 2);
  byte bitval = 8 >> (y & 3);  					// compute which bit will need set;
  
  // check green plane
  byte val = (ht1632_shadowram[chipNo][addr] & bitval) ? 1 : 0;
  if ((color & GREEN) != val)
  {
    if (val == 0)			// currently off, but we want it on
      ht1632_shadowram[chipNo][addr] |= bitval;
    else 				// currently on, but we want it off
      ht1632_shadowram[chipNo][addr] &= ~bitval;
    ht1632_senddata(chipNo+1, addr, ht1632_shadowram[chipNo][addr]);
  }

  // check red plane
  addr += 32;
  val =  (ht1632_shadowram[chipNo][addr] & bitval) ? 2 : 0;
  if ((color & RED) != val)
  {
    if (val == 0)
      ht1632_shadowram[chipNo][addr] |= bitval;
    else
      ht1632_shadowram[chipNo][addr] &= ~bitval;
    ht1632_senddata(chipNo+1, addr, ht1632_shadowram[chipNo][addr]);
  }
}


//*********************************************************************************************************
/*
 * ht1632_clear
 * clear the display, and the shadow memory, and the snapshot
 * memory.  This uses the "write multiple words" capability of
 * the chipset by writing all 32 bytes of memory without raising
 * the chipselect signal.
 */
void ht1632_clear(int8_t dispNo)
{
  if (dispNo < 0)
  {
    // clear our own shadow memory;
    memset(ht1632_shadowram, 0, sizeof(ht1632_shadowram));

    // clear the display memory;
    for (int8_t chipNo=1; chipNo<=CHIP_MAX; chipNo++)
    {
      ht1632_chipselect(chipNo);
      ht1632_writebits(HT1632_ID_WR, 1<<2);       // send ID: WRITE to RAM
      ht1632_writebits(0, 1<<6);                  // Send address
      for (int8_t i=0; i<32; i++)
      {
	ht1632_writebits(0, 1<<7);                // send 8 bits of data
      }
      ht1632_chipselect(0);
    }
    return;
  }
  // clear just the display specified
  // first the shadow memory
  memset(&ht1632_shadowram[dispNo * 4][0], 0, 256);
  // clear the display memory;
  for (int8_t chipNo=dispNo*4+1; chipNo<=dispNo*4+4; chipNo++)
  {
    ht1632_chipselect(chipNo);
    ht1632_writebits(HT1632_ID_WR, 1<<2);       // send ID: WRITE to RAM
    ht1632_writebits(0, 1<<6);                  // Send address
    for (int8_t i=0; i<32; i++)
    {
      ht1632_writebits(0, 1<<7);                // send 8 bits of data
    }
    ht1632_chipselect(0);
  }
}


//*********************************************************************************************************
/*
 * ht1632_copyToVideo(chipNo, Buffer);
 * copy 64 4 bit nibbles from the buffer to 1 chip of the display memory.
 * This uses the "write multiple words" capability of
 * the chipset by writing all 64 words of memory without raising
 * the chipselect signal.
 */
void ht1632_copyToVideo(byte chipNo, char* vbuffer)
{
  ht1632_chipselect(chipNo);
  ht1632_writebits(HT1632_ID_WR, 1<<2);		// send ID: WRITE to RAM
  ht1632_writebits(0, 1<<6);
  for (int16_t i=0; i < 64; i++)
  {
    ht1632_writebits(vbuffer[i], 1<<3);		// send 4 bits of data 64 times
  }
  ht1632_chipselect(0);
}


//*********************************************************************************************************
/*
 * snapshot_shadowram
 * Copy the shadow ram into the snapshot ram (the upper bits)
 * This gives us a separate copy so we can plot new data while
 * still having a copy of the old data.  snapshotram is NOT
 * updated by the plot functions (except "clear").
 */
void snapshot_shadowram()
{
  for (int8_t chipNo=0; chipNo<CHIP_MAX; chipNo++)
  {
    for (int8_t addr=0; addr<64; addr++)
    {
      // copy the video bits (lower 4) in the upper 4;
      byte val = ht1632_shadowram[chipNo][addr];
      ht1632_shadowram[chipNo][addr] = (val & 0x0F) | (val << 4);
    }
  }
}


//*********************************************************************************************************
/*
 * return the value of a pixel from the video memory (either BLACK, RED, GREEN, ORANGE);
 */
byte get_shadowram(coord_t x, int8_t y)
{
	return get_videoram(x, y, 0x08);					// get lower 4 bit;
}	


//*********************************************************************************************************
/*
 * get_snapshotram
 * get a pixel value from the snapshot ram instead of the actual video memory;
 * return BLACK, GREEN, RED or ORANGE;
 */
byte get_snapshotram(coord_t x, int8_t y)
{
	return get_videoram(x, y, 0x80);				// get higher 4 bit;
}


//*********************************************************************************************************
/*
 * return the value of a pixel from the video memory (either BLACK, RED, GREEN, ORANGE);
 */
byte get_videoram(coord_t x, int8_t y, byte whichBit)
{
//  int8_t chipNo = x / 16 + (y > 7 ? 2 : 0) ;
  int8_t chipNo = ((x & 0x10) >> 4) | ((y & 8) >> 2);
#if NUM_DISPLAYS > 1
  chipNo |= ((x & 0xffe0) >> 3);
#endif
  y &= 7;
  byte addr = ((x & 0xf) << 1) + (y >> 2);

  byte bitval = whichBit >> (y & 3);
  byte val = (ht1632_shadowram[chipNo][addr] & bitval) ? 1 : 0;
  val |= (ht1632_shadowram[chipNo][addr+32] & bitval) ? 2 : 0;
  return val;
}


//*********************************************************************************************************
/*
 * write the value of a pixel in the snapshot memory (either BLACK, RED, GREEN, ORANGE);
 */
void put_snapshotram(coord_t x, int8_t y, byte color)
{
//  int8_t chipNo = x / 16 + (y > 7 ? 2 : 0) ;
  int8_t chipNo = ((x & 0x10) >> 4) | ((y & 8) >> 2);
#if NUM_DISPLAYS > 1
  chipNo |= ((x & 0xffe0) >> 3);
#endif
  y &= 7;
  byte addr = ((x & 0xf) << 1) + (y >> 2);

  byte bitval = 0x80 >> (y & 3);

  // check green plane
  byte val = (ht1632_shadowram[chipNo][addr] & bitval) ? 1 : 0;
  if ((color & GREEN) != val)
  {
    if (val == 0)			// currently off, but we want it on
      ht1632_shadowram[chipNo][addr] |= bitval;
    else 				// currently on, but we want it off
      ht1632_shadowram[chipNo][addr] &= ~bitval;
  }

  // check red plane
  addr += 32;
  val = (ht1632_shadowram[chipNo][addr] & bitval) ? 2 : 0;
  if ((color & RED) != val)
  {
    if (val == 0)
      ht1632_shadowram[chipNo][addr] |= bitval;
    else
      ht1632_shadowram[chipNo][addr] &= ~bitval;
  }
}


//*********************************************************************************************************
/*
 * overlay the current display with one line of the snapshot memory;
 */
void overlayWithSnapshotHorizontal(int8_t y)
{
#ifdef RESTRICT_SCROLL_DISPLAY
	for (coord_t x = 0; x < 32; ++x)
#else
	for (coord_t x = 0; x < X_MAX; ++x)
#endif
		ht1632_plot(x, y, get_videoram(x, y, 0x80));
	
}


//*********************************************************************************************************
/*
 * overlay the current display with one column of the snapshot memory;
 */
void overlayWithSnapshotVertical(coord_t x)
{
	for (int8_t y = 0; y < Y_MAX; ++y)
		ht1632_plot(x, y, get_videoram(x, y, 0x80));
}	


//*********************************************************************************************************
/*
 * clear the snapshot memory;
 */
void clearSnapshot(int8_t dispNo)
{
    int8_t chipNo, maxChip;
    if (dispNo < 0)
    {
	chipNo = 0;
	maxChip = CHIP_MAX;
    } else {
	chipNo = dispNo * 4;
	maxChip = chipNo + 4;
    }

    while(chipNo < maxChip)
    {
	for (int8_t addr=0; addr < 64; addr++)
	    ht1632_shadowram[chipNo][addr] &= 0x0F;
	chipNo++;
    }
}

//*********************************************************************************************************
/*
 * ht1632_writebits
 * Write bits (up to 8) to h1632 on pins ht1632_data, ht1632_wrclk
 * Chip is assumed to already be chip-selected
 * Bits are shifted out from MSB to LSB, with the first bit sent
 * being (bits & firstbit), shifted till firsbit is zero.
 */
void ht1632_writebits (byte bits, byte firstbit)
{
	while (firstbit)
	{
		fWriteA(HT1632_WRCLK, LOW);	// digitalWrite(HT1632_WRCLK, LOW);

		if (bits & firstbit)
		{
			fWriteA(HT1632_DATA, HIGH);	// digitalWrite(HT1632_DATA, HIGH);
		} 
		else
		{
			fWriteA(HT1632_DATA, LOW);	// digitalWrite(HT1632_DATA, LOW);
		}

		fWriteA(HT1632_WRCLK, HIGH);	// digitalWrite(HT1632_WRCLK, HIGH);
		firstbit >>= 1;
	}
}


//*********************************************************************************************************
/*
 * ht1632_sendcmd
 * Send a command to the ht1632 chip.
 */
void ht1632_sendcmd (int8_t chipNo, byte command)
{
  ht1632_chipselect(chipNo);
  ht1632_writebits(HT1632_ID_CMD, 1<<2);  // send 3 bits of id: COMMMAND
  ht1632_writebits(command, 1<<7);  // send the actual command
  ht1632_writebits(0, 1); 	/* one extra dont-care bit in commands. */
  ht1632_chipselect(0);
}



//*********************************************************************************************************
/*
 * ht1632_senddata
 * send a nibble (4 bits) of data to a particular memory location of the
 * ht1632.  The command has 3 bit ID, 7 bits of address, and 4 bits of data.
 *    Select 1 0 1 A6 A5 A4 A3 A2 A1 A0 D0 D1 D2 D3 Free
 * Note that the address is sent MSB first, while the data is sent LSB first!
 * This means that somewhere a bit reversal will have to be done to get
 * zero-based addressing of words and dots within words.
 */
void ht1632_senddata (int8_t chipNo, byte address, byte data)
{
  ht1632_chipselect(chipNo);
  ht1632_writebits(HT1632_ID_WR, 1<<2);  // send ID: WRITE to RAM
  ht1632_writebits(address, 1<<6); // Send address
  ht1632_writebits(data, 1<<3); // send 4 bits of data
  ht1632_chipselect(0);
}




//*********************************************************************************************************
/*
 * Copy a character glyph from the myfont data structure to
 * display memory, with its upper left at the given coordinate
 * This is unoptimized and simply uses plot() to draw each dot.
 * (fc, Jan 30/2011) display character using the specified color;
 */
void ht1632_putchar(coord_t x, int8_t y, char c, byte color)
{
	// fonts defined for ascii 32 and beyond (index 0 in font array is ascii 32);
	byte charIndex;
	const unsigned char *fontptr;

	// replace undisplayable characters with blank;
	if (c < 32 || c > 127)
	{
		charIndex	=	0;
	}
	else
	{
		charIndex	=	c - 32;
	}

	fontptr = &myfont[charIndex][0];
	if (color & SMALL_CHAR)
	{
		color &= ~SMALL_CHAR;
		if (c >= 48 && c <= 57)
		{
			charIndex = c - 48;
			fontptr = &smallFont[charIndex][0];
		}
	}
	// move character definition, pixel by pixel, onto the display;
	// fonts are defined as one byte per row;
	for (int8_t row=0; row<8; row++)
	{
		byte rowDots	=	pgm_read_byte_near(fontptr + row);
		for (int8_t col=0; col<6; col++)
		{
#ifdef RESTRICT_SCROLL_DISPLAY
			if (x+col >= 32)
				continue;
#endif
			if (rowDots & (1<<(5-col)))
				ht1632_plot(x+col, y+row, color);
			else 
				ht1632_plot(x+col, y+row, color & PUTINSNAPSHOTRAM);
		}
	}
}


//*********************************************************************************************************
/*
 * Copy a character glyph from the proportional, largeFont data structure to
 * display memory, with its upper left at the given coordinate.
 * Font is 14 dots high and max 11 dots width.
 * This is unoptimized and simply uses plot() to draw each dot.
 */
coord_t ht1632_putLargeChar(coord_t x, int8_t y, char c, byte color)
{
	// fonts defined for ascii 32 and beyond (index 0 in font array is ascii 32);
	byte charIndex;
	int8_t col, row;
	
	// replace undisplayable characters with blank;
	if (c < 32 || c > 127)
	{
		charIndex	=	0;
	}
	else
	{
		charIndex	=	c - 32;
	}

	// move character definition, pixel by pixel, onto the display;
	// Fonts are defined as up to 14 bit per row and max. 11 columns;
	// first row is always zero to create the space between the characters;
	
	for (col=0; col < 11; ++col)					// max 11 columns;
	{
		uint16_t dots = pgm_read_word_near(&largeFont[charIndex][col]);
		if (dots == 0) 								// stop if all bits zero;
			break;
	
		for (row=0; row < 14; row++) 
		{
#ifdef RESTRICT_SCROLL_DISPLAY
			if (x+col >= 32)
				continue;
#endif
			if (dots & (0x4000 >> row))    			// max 14 rows;
				ht1632_plot(x+col, y+row, color);
			else 
				ht1632_plot(x+col, y+row, color & PUTINSNAPSHOTRAM);
		}
	}
	return x+col;
}


//*********************************************************************************************************
void displayStaticLine(char* text, int8_t y, byte color)
{
	coord_t len	=	strlen(text);
#ifdef RESTRICT_SCROLL_DISPLAY
	coord_t nx	=	(33 - 6*len) / 2;
#else
	// try to center the text;
	coord_t nx	=	(X_MAX+1 - 6*len) / 2;
#endif
	if (nx < 0)
	{
		nx = 0;		// text too long to fit on the screen;
	}
	for (coord_t i=0; i<len; i++)
	{
		ht1632_putchar(nx + i*6, y, text[i], color);
	}
}

//*********************************************************************************************************
// Just like displayStaticLine, but text is in Program space.
//
void displayStaticLine_P(char* text, int8_t y, byte color)
{
	coord_t len	=	strlen_P(text);
#ifdef RESTRICT_SCROLL_DISPLAY
	coord_t nx	=	(33 - 6*len) / 2;
#else
	// try to center the text;
	coord_t nx	=	(X_MAX+1 - 6*len) / 2;
#endif
	if (nx < 0)
	{
		nx = 0;		// text too long to fit on the screen;
	}
	for (coord_t i=0; i<len; i++)
	{
		ht1632_putchar(nx + i*6, y, pgm_read_byte_near(&text[i]), color);
	}
}


//*********************************************************************************************************
/* (fc, Aug 1/10)
 * display a big digit, defined on a 6x12 grid;
 * the purpose is to display the time H12:MM on the 24x16 matrix;
 */
// (rp, Oct 2010) modified to use multiple big fonts;
void ht1632_putBigDigit(coord_t x, int8_t y, int8_t digit, int8_t fontNbr, byte color, int8_t columns)
{
	// move character definition, pixel by pixel, onto the display;
	// a big digit is defined as 12 rows, one byte per row;

    int fontOffset = fontNbr * CHARS_IN_FONT;

	for (int8_t row=0; row < BYTES_PER_CHARS; row++)
	{
		byte rowDots = pgm_read_byte_near(&bigFont[digit + fontOffset][row]);
		for (int8_t col=0; col<columns; col++)
		{
			if (rowDots & (1<<((columns - 1) - col)))
				ht1632_plot(x+col, y+row, color);
			else 
				ht1632_plot(x+col, y+row, color & PUTINSNAPSHOTRAM);
		}
	}
}


//*********************************************************************************************************
/* (fc, Jun 10/2011)
 * load and display a given bitmap (defined in bitmaps.h);
 */
void ht1632_putBitmap(coord_t x, int8_t y, byte indexBmp, byte color)
{
  if (color)
	// move character definition, pixel by pixel, onto the display;
	// bitmap has 14 rows x 14 columns, one WORD per row;

	for (int8_t row=0; row < 14; row++)
	{
		uint16_t rowDots = pgm_read_word_near(&bitmap[indexBmp][row]);
		for (int8_t col=0; col<14; col++)
		{
#ifdef RESTRICT_SCROLL_DISPLAY
			if (x+col >= 32)
				continue;
#endif
			if (rowDots & (1<<(13-col)))
				ht1632_plot(x+col, y+row, color);
			else
				// little hack for pacman mode; comment for any other bitmap;
				if (col > 0 || (row!=6 && row!=7))
					ht1632_plot(x+col, y+row, color & PUTINSNAPSHOTRAM);
		}
	}
}


//*********************************************************************************************************
/*
 * Copy a character glyph from the tinyFont data structure to
 * display memory, with its upper left at the given coordinate
 * This is unoptimized and simply uses plot() to draw each dot.
 */
void ht1632_putTinyChar(coord_t x, int8_t y, char c, byte color)
{
	// fonts defined for ascii 32 and beyond (index 0 in font array is ascii 32);
	byte charIndex;

	// replace undisplayable characters with blank;
	if (c < 32 || c > 126)
	{
		charIndex	=	0;
	}
	else
	{
		charIndex	=	c - 32;
	}

	// move character definition, pixel by pixel, onto the display;
	// fonts are defined as one byte per row;
	for (int8_t row=0; row<8; row++)
	{
		byte rowDots	=	pgm_read_byte_near(&tinyFont[charIndex][row]);
		for (int8_t col=0; col<4; col++)
		{
			if (rowDots & (1<<(3-col)))
				ht1632_plot(x+col, y+row, color);
			else 
				ht1632_plot(x+col, y+row, color & PUTINSNAPSHOTRAM);
		}
	}
}


//*********************************************************************************************************
/*
 * Display a string with the tiny font at the given coordinates.
 */
void ht1632_putTinyString(coord_t x, int8_t y, const char* str, byte color)
{
  int len = strlen(str);
  for (int i=0; i<len; i++)
  {
    // start one column to the left (the first column of each character is always empty);
    ht1632_putTinyChar(x+i*4-1, y, str[i], color);
  }
}


//*********************************************************************************************************
/*
 * Display a string with regular font at the given coordinates.
 */
void ht1632_putString(coord_t x, int8_t y, const char* str, byte color)
{
  int len = strlen(str);
  for (int i=0; i<len; i++)
  {
    // start one column to the left (the first column of each character is always empty);
    ht1632_putchar(x+i*6-1, y, str[i], color);
  }
}


//*********************************************************************************************************
//
void setBrightness(byte nLevel)
{
  if (nLevel > 5)
      nLevel = 5;

  for (int8_t i=1; i<=CHIP_MAX; i++)
    ht1632_sendcmd(i, HT1632_CMD_PWM + nLevel*3);

}


//*********************************************************************************************************
/*
 * Draw a line between two points using the bresenham algorithm.
 * This particular bit of code is copied from
 * http://en.wikipedia.org/wiki/Bresenham's_line_algorithm
 * The 'abs()' calls were brought in-line, the '-dy' comparison
 * was replaced with 'ndy' & changing the sign, and the loop
 * was re-arranged so there is only one exit.
 *
 */
void ht1632_line(coord_t x0, int8_t y0, coord_t x1, int8_t y1, byte val)
{
	coord_t dx;			// The difference between the x's
	coord_t ndy;			// The negative difference between the y's
	coord_t sx, sy, err, e2;

	dx = x1 - x0;
	if (dx < 0) {				// The x-values are decreasing
		sx = -1;
		dx = -dx;
	} else {				// The x-values are increasing
		sx =  1;
	}

	ndy = y0 - y1;
	if (ndy > 0) {				// The y-values are decreasing
		sy = -1;
		ndy = -ndy;
	} else {				// The y-values are increasing
		sy =  1;
	}
	err = dx + ndy;

	while((x0 != x1) || (y0 != y1)) {
		ht1632_plot(x0, y0, val);	// Draw the current pixel
		e2 = err * 2;
		if (e2 > ndy) {
			err += ndy;
			x0 += sx;
		}
		if ((e2 < dx) && (y0 != y1)) {
			err += dx;
			y0 += sy;
		}
	}
	ht1632_plot(x0, y0, val);		// Draw the last pixel
}

