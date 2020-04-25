//*********************************************************************************************************
//
// AppBase.cpp
//
//*********************************************************************************************************


#include "AppBase.h"


void CAppBase::recordStartAppTime()
{
	m_appStartTime = millis();
}


//*********************************************************************************************************
// Check if millisec miliseconds have passed since gAppStartTime and allow for millis() to run over to zero;
// See www.arduino.cc/playground/Code/TimingRollover for explanation

boolean CAppBase::hasAppTimePassed(unsigned long millisec)
{
	if ((long) (millis() - ((unsigned long)(m_appStartTime + millisec))) >= 0)
		return true;
	else
		return false;
}

