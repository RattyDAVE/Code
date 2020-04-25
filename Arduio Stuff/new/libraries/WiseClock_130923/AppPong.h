/*
 *********************************************************************************************************
 * AppPong.h, part of Wise Clock 3 library, by FlorinC (http://timewitharduino.blogspot.com/);
 *
 * Code copied and adapted from Nick (http://123led.wordpress.com/about/);
 *
 *********************************************************************************************************
 */


#ifndef _APP_PONG_H_
#define _APP_PONG_H_


#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include "AppBase.h"


class CAppPong : public CAppBase
{
private:
  coord_t ballpos_x;
  coord_t ballvel_x;
  int16_t ballpos_y;	// scaled by 256
  int16_t ballvel_y;	// scaled by 256
  coord_t erase_x;  //holds ball old pos so we can erase it, set to blank area of screen initially.
  int8_t erase_y;
  int8_t bat1_y;  //bat starting y positions
  int8_t bat2_y;  
  int8_t bat1_target_y;  //bat targets for bats to move to
  int8_t bat2_target_y;
  byte bat1_update;  //flags - set to update bat position
  byte bat2_update;
  byte bat1miss, bat2miss; //flags set on the minute or hour that trigger the bats to miss the ball, thus upping the score to match the time.
  byte restart;   //game restart flag - set to 1 initially to setup 1st game
  byte start_hour;  // hour when game started
  byte start_minute; // minute when game started

  byte pong_get_ball_endpoint(coord_t tempballpos_x, int16_t  tempballpos_y, coord_t  tempballvel_x, int16_t tempballvel_y);

public:
  virtual void init(byte mode=0);
  virtual int16_t run();
};


extern CAppPong appPong;


#endif  // _APP_PONG_H_
