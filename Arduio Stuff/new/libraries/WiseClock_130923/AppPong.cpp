/*
 *********************************************************************************************************
 * AppPong.cpp, part of Wise Clock 3 library, by FlorinC (http://timewitharduino.blogspot.com/);
 *
 * Code copied and adapted from Nick (http://123led.wordpress.com/about/);
 *
 * Apr 07, 2012 (mcm)  parameterized colors
 * Apr 10, 2012 (mcm)  Honor 12hr/24hr flag
 * Apr 12, 2012 (mcm)  parameterized for X_MAX, Y_MAX, BAT_HEIGHT
 * Apr 13, 2012 (mcm)  removed floating point code in favor of scaled integers
 * Apr 14, 2012 (mcm)  fixed prediction code, flick code
 * May 28, 2012 (mcm)  restructure to eliminate delay() calls
 * Aug 14, 2013 (mcm)  use get12Hour() instead of calculating 12-hour mode
 *********************************************************************************************************
 */


#include "UserConf.h"
#ifdef WANT_APP_PONG

#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif
#include "HT1632.h"

#ifdef RESTRICT_SCROLL_DISPLAY
 #undef X_MAX
 #define X_MAX 32
#endif
#include "AppPong.h"
#include "AlarmClock.h"


// (fc) dependent on the screen size;
#define BAT1_X 2		// Pong left bat x pos (this is where the ball collision occurs, the bat is drawn 1 behind these coords)
#define BAT2_X (X_MAX-3)	// Pong right bat x pos (this is where the ball collision occurs, the bat is drawn 1 behind these coords)

#define BAT_HEIGHT 5	// height of bat

#define COLOR_SCORE	RED
#define COLOR_PITCH	GREEN
#define COLOR_BAT	ORANGE
#define COLOR_BALL	GREEN


void CAppPong::init(byte mode /*=0*/)
{
	erase_x = 10;  //holds ball old pos so we can erase it, set to blank area of screen initially.
	erase_y = 10;
	bat1_y = 5;  //bat starting y positions
	bat2_y = 5;  
	bat1_target_y = 5;  //bat targets for bats to move to
	bat2_target_y = 5;
	bat1_update = 1;  //flags - set to update bat position
	bat2_update = 1;
	restart = 1;   //game restart flag - set to 1 initially to setup 1st game

	clearDisplay();
  
	// draw pitch centre line;
	for (int8_t i = 0; i <Y_MAX; i++) {
		if ((i & 1) == 0 ) { //plot point if an even number
			ht1632_plot(X_MAX/2, i, COLOR_PITCH); 
		}
	} 
}


//*****************************************************************************************************

byte CAppPong::pong_get_ball_endpoint(coord_t tempballpos_x, int16_t tempballpos_y, coord_t tempballvel_x, int16_t tempballvel_y)
{
  //run prediction until ball hits bat
  while (tempballpos_x > BAT1_X && tempballpos_x < BAT2_X  ){     
    tempballpos_x = tempballpos_x + tempballvel_x;
    tempballpos_y = tempballpos_y + tempballvel_y;
    //check for collisions with top / bottom
    if (tempballpos_y <= 0){
      tempballpos_y = 0;
      tempballvel_y = tempballvel_y * -1;
    } else if (tempballpos_y >= ((Y_MAX-1) * 256)){
      tempballpos_y = (Y_MAX-1) * 256;
      tempballvel_y = tempballvel_y * -1;
    }    
  }  
  return tempballpos_y >> 8; 
}


//*****************************************************************************************************
// this function is called as part of the main loop;
//
int16_t CAppPong::run()
{
    //if restart flag is 1, setup a new game
    if (restart)
    {
      //erase ball pos
      ht1632_plot(erase_x, erase_y, 0);
      
      //update score / time
      byte mins = alarmClock.minute;
      byte hours = alarmClock.get12Hour();
        
      char buffer[3];

      itoa(hours,buffer,10);
      // fix - as otherwise if num has leading zero, e.g. "03" hours, itoa coverts this to chars with space "3 ". 
      if (hours < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      ht1632_putchar((X_MAX/2)-11, -1, buffer[0], COLOR_SCORE|SMALL_CHAR);
      ht1632_putchar((X_MAX/2)- 6, -1, buffer[1], COLOR_SCORE|SMALL_CHAR);
     
     
      itoa(mins,buffer,10); 
      if (mins < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      } 
      ht1632_putchar((X_MAX/2)+ 1, -1, buffer[0], COLOR_SCORE|SMALL_CHAR);
      ht1632_putchar((X_MAX/2)+ 6, -1, buffer[1], COLOR_SCORE|SMALL_CHAR);  
           
      // set ball start pos
      ballpos_x = X_MAX/2;
      ballpos_y = random (4,Y_MAX-4) << 8;

      // pick random ball direction
      if (random(0,2) > 0) {
        ballvel_x = 1; 
      } else {
        ballvel_x = -1;
      }
      if (random(0,2) > 0) {
        ballvel_y =  random(100, 150); 
      } else {
        ballvel_y = -random(100, 150);
      }
      // draw bats in initial positions
      bat1miss = 0; 
      bat2miss = 0;
      // reset game restart flag
      restart = 0;
      // save current hour, minute
      start_hour = alarmClock.hour;
      start_minute = alarmClock.minute;
      //short wait
      return(1500);
    }

    if (start_hour != alarmClock.hour){
      bat2miss = 1;	// if the hour has changed, flag bat 2 (right side) to miss
    } else if (start_minute != alarmClock.minute) {
      bat1miss = 1;	// if the minute has changed, flag bat 1 (left side) to miss
    }

    //AI - we run 2 sets of 'AI' for each bat to work out where to go to hit the ball back 

    // Left bat AI
    // when the ball is closer to the left bat and heading that way, run the ball maths to find out where the ball will land
    if (ballpos_x == ((X_MAX/2)-1) && ballvel_x < 0) {
      byte end_ball_y = pong_get_ball_endpoint(ballpos_x, ballpos_y, ballvel_x, ballvel_y);
             
      //if the miss flag is set,  then the bat needs to miss the ball when it gets to end_ball_y
      if (bat1miss){
        if (end_ball_y > (Y_MAX/2)){
	  bat1_target_y = end_ball_y - BAT_HEIGHT - random (1,3);	// near miss appearent low part of bat
	} else {
	  bat1_target_y = end_ball_y + random (2,4);			// near miss appearent high part of bat
	}      
      } else {
        //if the miss flag isn't set,  set bat target to ball end point with some randomness so its not always hitting top of bat
	bat1_target_y = end_ball_y - random(0,BAT_HEIGHT);        
	//check not less than 0
	if (bat1_target_y < 0){
	  bat1_target_y = 0;
	} else if (bat1_target_y > (Y_MAX - BAT_HEIGHT)){
	  bat1_target_y = (Y_MAX - BAT_HEIGHT);
	} 
      }
    }
    // When further away, just tell the bat to move to the height of the ball when we get to a random location.
    else if (ballpos_x == random(X_MAX/2 + 4, X_MAX/2 + 11)){// && ballvel_x < 0) {
      bat1_target_y = ballpos_y >> 8;
    }
        
        
    // Right bat AI
    // when the ball is closer to the right bat and heading that way, run the ball maths to find out where the ball will land
    if (ballpos_x == ((X_MAX/2)+1) && ballvel_x > 0) {
      byte end_ball_y = pong_get_ball_endpoint(ballpos_x, ballpos_y, ballvel_x, ballvel_y);

      //if flag set to miss, move bat out way of ball
      if (bat2miss){
        //if ball end point above 8 then move bat down, else move it up- so either way it misses
        if (end_ball_y > (Y_MAX/2)){
          bat2_target_y = end_ball_y - BAT_HEIGHT - random (1,3);	// near miss appearent low part of bat 
        } else {
          bat2_target_y = end_ball_y + random (2,4);			// near miss appearent high part of bat
        }      
      } else {
        //set bat target to ball end point with some randomness 
        bat2_target_y = end_ball_y - random (0,BAT_HEIGHT);
        //ensure target between 0 and 15
        if (bat2_target_y < 0){
          bat2_target_y = 0;
        } else if (bat2_target_y > (Y_MAX - BAT_HEIGHT)){
          bat2_target_y = (Y_MAX - BAT_HEIGHT);
        } 
      }
    }
    // When further away, just tell the bat to move to the height of the ball when we get to a random location.
    else if (ballpos_x == random(X_MAX/2 - 11, X_MAX/2 - 4)){//  && ballvel_x > 0) {
      bat2_target_y = ballpos_y >> 8;
    }


    //move bat 1 towards target    
    //if bat y greater than target y move down until hit 0 (dont go any further or bat will move off screen)
    if (bat1_y > bat1_target_y) {
#if Y_MAX > X_MAX/2
      if ((ballvel_x < 0) && (ballpos_x - BAT1_X) < (bat1_y - bat1_target_y))
	bat1_y -= 2;
      else
#endif
	bat1_y--;
      if (bat1_y < 0) bat1_y = 0;
      bat1_update = 1;
    }

    //if bat y less than target y move up until hit 10 (as bat is 6)
    else if (bat1_y < bat1_target_y) {
#if Y_MAX > X_MAX/2
      if ((ballvel_x < 0) && (ballpos_x - BAT1_X) < (bat1_target_y - bat1_y))
	bat1_y += 2;
      else
#endif
	bat1_y++;
      if (bat1_y > (Y_MAX - BAT_HEIGHT))
	bat1_y = (Y_MAX - BAT_HEIGHT);
      bat1_update = 1;
    }

    //draw bat 1
    if (bat1_update){
      for (int8_t i = 0; i < Y_MAX; i++){
        if (i - bat1_y < BAT_HEIGHT &&  i - bat1_y > -1){
          ht1632_plot(BAT1_X-1, i , COLOR_BAT);
          ht1632_plot(BAT1_X-2, i , COLOR_BAT);
        } else {
          ht1632_plot(BAT1_X-1, i , 0);
          ht1632_plot(BAT1_X-2, i , 0);
        }
      } 
    }


    //move bat 2 towards target (dont go any further or bat will move off screen)

    //if bat y greater than target y move down until hit 0
    if (bat2_y > bat2_target_y) {
#if Y_MAX > X_MAX/2
      if ((ballvel_x > 0) && (BAT2_X - ballpos_x) < (bat2_y - bat2_target_y))
	bat2_y -= 2;
      else
#endif
	bat2_y--;
      if (bat2_y < 0) bat2_y = 0;
      bat2_update = 1;
    }

    //if bat y less than target y move up until hit max of 10 (as bat is 6)
    else if (bat2_y < bat2_target_y) {
#if Y_MAX > X_MAX/2
      if ((ballvel_x > 0) && (BAT2_X - ballpos_x) < (bat2_target_y - bat2_y))
	bat2_y += 2;
      else
#endif
	bat2_y++;
      if (bat2_y > (Y_MAX - BAT_HEIGHT))
	bat2_y = (Y_MAX - BAT_HEIGHT);
      bat2_update = 1;
    }

    //draw bat2
    if (bat2_update){
      for (int8_t i = 0; i < Y_MAX; i++){
        if (i - bat2_y < BAT_HEIGHT && i - bat2_y > -1){
          ht1632_plot(BAT2_X+1, i , COLOR_BAT);
          ht1632_plot(BAT2_X+2, i , COLOR_BAT);
        } else {
          ht1632_plot(BAT2_X+1, i , 0);
          ht1632_plot(BAT2_X+2, i , 0);
        }
      } 
    }

    //update the ball position using the velocity
    ballpos_x =  ballpos_x + ballvel_x;
    ballpos_y =  ballpos_y + ballvel_y;

    //check ball collision with top and bottom of screen and reverse the y velocity if either is hit
    if (ballpos_y <= 0 ){
      ballvel_y = ballvel_y * -1;
      ballpos_y = 0; //make sure value goes no less that 0
    } else if (ballpos_y >= ((Y_MAX-1) * 256)){
      ballvel_y = ballvel_y * -1;
      ballpos_y = (Y_MAX-1) * 256; //make sure value goes no more than 15
    }
    
     //check for ball collision with bat1. check ballx is same as batx
     //and also check if bally lies within width of bat i.e. baty to baty + 6. We can use the exp if(a < b && b < c) 
     if (ballpos_x == BAT1_X && (bat1_y <= (ballpos_y >> 8) && (ballpos_y >> 8) <= bat1_y + (BAT_HEIGHT-1)) ) { 
       
       ballvel_x = ballvel_x * -1;
       //random if bat flicks ball to return it - and therefor changes ball velocity
       if(random(0,3) != 0) { //not true = no flick - just straight rebound and no change to ball y vel
         bat1_update = 1;
         byte flick;  //0 = up, 1 = down.
   
         //if bat 1 or 2 away from top only flick down
         if (bat1_y <=1 ){
           flick = 0;   //move bat down 1 or 2 pixels 
         } else if (bat1_y >= (Y_MAX-BAT_HEIGHT-1)){
         //if bat 1 or 2 away from bottom only flick up
           flick = 1;  //move bat up 1 or 2 pixels 
         } else {
           flick = random(0,2);   //pick a random dir to flick - up or down
         }
         
         switch (flick) {
           //flick up
           case 0:
             bat1_target_y += random(1,3);
	     if (ballvel_y < 0 && ballvel_y > -500)
	       ballvel_y -= random(30, 51);
	     else if (ballvel_y >= 0 && ballvel_y < 500)
	       ballvel_y += random(30, 51);
             break;
            
            //flick down
           case 1:   
             bat1_target_y -= random(1,3);
	     if (ballvel_y < -50)
	       ballvel_y += random(30, 51);
	     else if (ballvel_y > 50)
	       ballvel_y -= random(30, 51);
             break;
           }
         }
       }
       
       
       //check for ball collision with bat2. check ballx is same as batx
       //and also check if bally lies within width of bat i.e. baty to baty + 6. We can use the exp if(a < b && b < c) 
       if (ballpos_x == BAT2_X && (bat2_y <= (ballpos_y >> 8) && (ballpos_y >> 8) <= bat2_y + (BAT_HEIGHT-1)) ) { 
       
       ballvel_x = ballvel_x * -1;
       //random if bat flicks ball to return it - and therefor changes ball velocity
       if(random(0,3) != 0) { //not true = no flick - just straight rebound and no change to ball y vel
         bat2_update = 1;
         byte flick;  //0 = up, 1 = down.
           
         //if bat 1 or 2 away from top only flick down
         if (bat2_y <= 1 ){
           flick = 0;  //move bat up 1 or 2 pixels 
         } else if (bat2_y >= (Y_MAX-BAT_HEIGHT-1)){
         //if bat 1 or 2 away from bottom only flick up
           flick = 1;   //move bat down 1 or 2 pixels 
         } else {
           flick = random(0,2);   //pick a random dir to flick - up or down
	 }
         
         switch (flick) {
           //flick up
           case 0:
             bat2_target_y += random(1,3);
	     if (ballvel_y < 0 && ballvel_y > -500)
	       ballvel_y -= random(30, 51);
	     else if (ballvel_y >= 0 && ballvel_y < 500)
	       ballvel_y += random(30, 51);
             break;
            
            //flick down
           case 1:   
             bat2_target_y -= random(1,3);
	     if (ballvel_y < -50)
	       ballvel_y += random(30, 51);
	     else if (ballvel_y > 50)
	       ballvel_y -= random(30, 51);
             break;
           }
         }
       }

    //plot the ball on the screen
    coord_t plot_x = ballpos_x;
    int8_t plot_y = ballpos_y >> 8;

    // not needed, as we're only looking at shadow ram, not snapshot ram
    //snapshot_shadowram();	//take a snapshot of all the led states
    
    ht1632_plot(erase_x, erase_y, 0);   // erase old ball position
    // Only plot the ball if the LED at the ball position is currently off.
    // Only update the erase positions if the ball is plotted.
    // if the ball is not plotted, on the next loop the same point will be erased rather than this point which shouldn't be erased.
    if (get_shadowram(plot_x, plot_y) == BLACK){
      // plot the ball and save erase position
      ht1632_plot(plot_x, plot_y, COLOR_BALL);     
      //reset erase to new pos
      erase_x = plot_x; 
      erase_y = plot_y;
    }

    //check if a bat missed the ball. if it did, reset the game.
    if (ballpos_x <= (BAT1_X-2) || ballpos_x >= (BAT2_X+2)){
      restart = 1; 
    } 
    return(35);
}


CAppPong appPong;
#endif
