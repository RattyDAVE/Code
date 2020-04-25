// AppVu.cpp

#include "UserConf.h"
#ifdef WANT_APP_VU

#include "AppVu.h"
#include "HT1632.h"


//----------------------------------------------------
// connections to MSGEQ7;
//
int analogPin = 0;	// input from MSGEQ7 chip
int strobePin = 19;	// strobe is attached to digital pin 2
int resetPin = 18;	// reset is attached to digital pin 3



void CAppVu::init(byte mode/*=0*/)
{
  uint8_t tmp = 1<<JTD;		// Disable JTAG
  MCUCR = tmp;			// Disable JTAG
  MCUCR = tmp;			// Disable JTAG

  // MSGEQ7;
  pinMode(analogPin, INPUT);
  pinMode(strobePin, OUTPUT);
  pinMode(resetPin, OUTPUT);
  analogReference(DEFAULT);

  digitalWrite(resetPin, LOW);
  digitalWrite(strobePin, HIGH);


  clearDisplay();
}


// show 2-column bars for each of the 7 channels;
int16_t CAppVu::run()
{

	clearDisplay();
	
	digitalWrite(resetPin, HIGH);
	digitalWrite(resetPin, LOW);

	for (int i = 0; i < 7; i++)
	{
		digitalWrite(strobePin, LOW);
		delayMicroseconds(30); // to allow the output to settle
		spectrumValue[i] = analogRead(analogPin);
		digitalWrite(strobePin, HIGH);

                // try to eliminate some noise;
		if (spectrumValue[i] < 100)
                    spectrumValue[i] = 0;

		int x = i*5;
		int vu = spectrumValue[i]/64;
                if (vu <= 8)
                {
                  ht1632_line(x, 15, x, 15-vu, GREEN);
                  ht1632_line(x+1, 15, x+1, 15-vu, GREEN);
                }
                else if (vu <= 12)
                {
                  ht1632_line(x, 15, x, 8, GREEN);
                  ht1632_line(x+1, 15, x+1, 8, GREEN);
                  ht1632_line(x, 7, x, 15-vu, ORANGE);
                  ht1632_line(x+1, 7, x+1, 15-vu, ORANGE);
                }
                else
                {
                  ht1632_line(x, 15, x, 8, GREEN);
                  ht1632_line(x+1, 15, x+1, 8, GREEN);
                  ht1632_line(x,  7, x, 4, ORANGE);
                  ht1632_line(x+1,7, x+1, 4, ORANGE);
                  ht1632_line(x,  3, x, 16-vu, RED);
                  ht1632_line(x+1,3, x+1, 16-vu, RED);
                }
	}
}


CAppVu appVu;

#endif
