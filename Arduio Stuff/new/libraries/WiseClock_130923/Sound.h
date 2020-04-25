// Sound.h

#ifndef _SOUND_H_
#define _SOUND_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif


//*	Apr  1,	2010	<MLS> modified to use the non-C++ version of tone
//* May 29, 2011    <rp>  added void soundChimeShort(); void soundChimeLong();
//* Aug 14, 2013    <mcm> added void soundNote();
//*	tone is included in WProgram.h
#include "ToneNotes.h"

void setupSpeaker();
void soundAlarm();
void silenceAlarm();
void soundChimeShort();
void soundChimeLong();
void checkSound(unsigned long ms);
void beep();
void soundNote(uint8_t note, uint16_t dur);

#endif  // _SOUND_H_


