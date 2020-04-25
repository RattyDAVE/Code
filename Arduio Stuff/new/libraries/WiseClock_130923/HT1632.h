/*
 *********************************************************************************************************
 * HT1632.h - defintions for Holtek HT1632 LED driver.
 *
 * Apr/10 by FlorinC (http://timewitharduino.blogspot.com/)
 *   Copyrighted and distributed under the terms of the Berkeley license
 *   (copy freely, but include this notice of original authors.)
 *
 * Adapted after HT1632 library by Bill Westfield ("WestfW") (http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1225239439/0);
 *
 *********************************************************************************************************
 */

//*********************************************************************************************************
//*	Edit History, started April, 2010
//*	please put your initials and comments here anytime you make changes
//*********************************************************************************************************
//* Apr 15/10 (fc) created file, by restructuring Wise4Sure.pde;
//* Oct 10/10 (rp) adapted ht1632_putBigDigit() for multiple fonts;
//* Jan 29/11 (fc) modified for Wise Clock 3 (3216 bicolor display);
//* Jun 20/11 (rp) added color and columns parameter to putBigDigit(); 
//* Jun 20/11 (fc) added setBrightness(), ht1632_plot() now uses int instead of byte; 
//* Oct 15/11 (rp) added PUTINSNAPSHOTRAM, overlayWithSnapshotHorizontal(), overlayWithSnapshotVertical();
//* Apr 25/12 (rp) added ht1632_copyToVideo(chipNo, Buffer);
//* Apr 26/12 (mcm) changed shadowram[64][4] to shadowram[4][64] to better reflect hardware layout
//*                 saved ~200 bytes of program space!
//* May 05/12 (mcm) use int8_t or int16_t instead of int where appropriate
//* May 20/12 (mcm) #define RESTRICT_SCROLL_DISPLAY to limit scrolls to only one display
//*                 used when day/night graph is on 2nd display, scroll message on 1st display
//* May 28/12 (mcm) restructured to eliminate as many delay() calls as possible
//* Jul 02/12 (mcm) add #define SMALL_CHAR, combine putSmallChar() and putchar()
//* Sep 10/13 (mcm) move common config options to UserConf.h
//*
//*********************************************************************************************************


#ifndef _HT1632_
#define _HT1632_

#include "UserConf.h"

#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#define CHIP_MAX	(NUM_DISPLAYS * 4)
#define X_MAX		(NUM_DISPLAYS * 32)
#define Y_MAX		16

// possible values for a pixel;
#define BLACK  0
#define GREEN  1
#define RED    2
#define ORANGE 3

#define SMALL_CHAR	0x20			// if set use smaller characters for 0-9
#define PUTINSNAPSHOTRAM 0x10			// if set write to snapshot memory instead of shadow ram.;
#define CLEARSNAPSHOTRAM true			// used in overlay functions;
#define NOCLEARSNAPSHOTRAM false


/*
#define plot(x,y,v)  ht1632_plot(x,y,v)
#define cls          ht1632_clear
*/

#if NUM_DISPLAYS < 2
 #undef RESTRICT_SCROLL_DISPLAY
#endif

#if NUM_DISPLAYS > 3
  #define coord_t  int16_t
#else
  #define coord_t  int8_t
#endif

#if defined(RESTRICT_SCROLL_DISPLAY) || (NUM_DISPLAYS < 2)
 #define clearDisplay() ht1632_clear(0)
 // how much to pad scrolling lines
 #define BLANK_PREFIX	6
#else
 #define clearDisplay() ht1632_clear(-1)
 #define BLANK_PREFIX	(NUM_DISPLAYS * 6)
 #define MULTI_DISPLAY_SCROLL
#endif


/*
 * commands written to the chip consist of a 3 bit "ID", followed by
 * either 9 bits of "Command code" or 7 bits of address + 4 bits of data.
 */
#define HT1632_ID_CMD 4		/* ID = 100 - Commands */
#define HT1632_ID_RD  6		/* ID = 110 - Read RAM */
#define HT1632_ID_WR  5		/* ID = 101 - Write RAM */
#define HT1632_ID_BITS (1<<2)   /* IDs are 3 bits */

#define HT1632_CMD_SYSDIS 0x00	/* CMD= 0000-0000-x Turn off oscil */
#define HT1632_CMD_SYSON  0x01	/* CMD= 0000-0001-x Enable system oscil */
#define HT1632_CMD_LEDOFF 0x02	/* CMD= 0000-0010-x LED duty cycle gen off */
#define HT1632_CMD_LEDON  0x03	/* CMD= 0000-0011-x LEDs ON */
#define HT1632_CMD_BLOFF  0x08	/* CMD= 0000-1000-x Blink ON */
#define HT1632_CMD_BLON   0x09	/* CMD= 0000-1001-x Blink Off */
#define HT1632_CMD_SLVMD  0x10	/* CMD= 0001-00xx-x Slave Mode */
#define HT1632_CMD_MSTMD  0x14	/* CMD= 0001-01xx-x Master Mode */
#define HT1632_CMD_RCCLK  0x18	/* CMD= 0001-10xx-x Use on-chip clock */
#define HT1632_CMD_EXTCLK 0x1C	/* CMD= 0001-11xx-x Use external clock */
#define HT1632_CMD_COMS00 0x20	/* CMD= 0010-ABxx-x commons options */
#define HT1632_CMD_COMS01 0x24	/* CMD= 0010-ABxx-x commons options */
#define HT1632_CMD_COMS10 0x28	/* CMD= 0010-ABxx-x commons options */
#define HT1632_CMD_COMS11 0x2C	/* CMD= 0010-ABxx-x commons options */
#define HT1632_CMD_PWM    0xA0	/* CMD= 101x-PPPP-x PWM duty cycle */
#define HT1632_CMD_BITS (1<<7)



extern byte ht1632_shadowram[CHIP_MAX][64];		// our copy of the display's RAM


void		snapshot_shadowram();
byte		get_snapshotram(coord_t x, int8_t y);
byte		get_shadowram(coord_t x, int8_t y);
byte 		get_videoram(coord_t x, int8_t y, byte which4Bits);
void 		put_snapshotram(coord_t x, int8_t y, byte color);
void 		overlayWithSnapshotHorizontal(int8_t y);
void 		overlayWithSnapshotVertical(coord_t x);
void		clearSnapshot(int8_t dispNo);

void		ht1632_setup();
void		ht1632_clear(int8_t dispNo);

void		ht1632_writebits(byte bits, byte firstbit);
void		ht1632_sendcmd(int8_t chipNo, byte command);
void		ht1632_senddata(int8_t chipNo, byte address, byte data);
void		ht1632_copyToVideo(byte chipNo, char* vbuffer);
void		ht1632_plot        (coord_t x, int8_t y, byte color);
void		ht1632_putchar     (coord_t x, int8_t y, char c, byte color);
void		ht1632_putTinyChar (coord_t x, int8_t y, char c, byte color);
coord_t		ht1632_putLargeChar(coord_t x, int8_t y, char c, byte color);
void		ht1632_putBigDigit (coord_t x, int8_t y, int8_t digit, int8_t fontNbr, byte color, int8_t columns);
void		ht1632_putBitmap   (coord_t x, int8_t y, byte indexBmp, byte color=ORANGE);
void		ht1632_putString    (coord_t x, int8_t y, const char* str, byte color);
void		ht1632_putTinyString(coord_t x, int8_t y, const char* str, byte color);

void		displayStaticLine(char* text, int8_t y, byte color);
void		displayStaticLine_P(char* text, int8_t y, byte color);
void		setBrightness(byte level);

void		ht1632_line(coord_t x1, int8_t y1, coord_t x2, int8_t y2, byte val);

#endif  // _HT1632_

