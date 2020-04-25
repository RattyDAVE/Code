//**********************************************************************************************************************
// AppBase.h - (fc, Sep 1, 2013) created 
//
// All App classes are derived from this base class for the purpose of implementing polymorphism.
// This way, the huge switch statements in WiseClock.cpp will be replaced by pCrtApp->init(), pCrtApp->run().
// Also, common members and functions can be set or modified in the base class.
//
//**********************************************************************************************************************


#ifndef _APP_BASE_H_
#define _APP_BASE_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif



class CAppBase
{
public:
  virtual void init(byte mode=0) = 0;
  virtual int16_t run() = 0;

  virtual void recordStartAppTime();

  virtual boolean hasAppTimePassed(unsigned long millisec=0);

  byte getCrtApp();
  void setCrtApp(byte crtApp);

private:
  // record the app start time, if we need to exit it after a while (e.g. Life, Demo);
  unsigned long	m_appStartTime;
};


#endif  // _APP_BASE_H_


