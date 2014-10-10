/*
 * map.c
 * 
 * Copyright 2014 Dave Pucknell <dave@vm-lubuntu>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */


#include <stdio.h>
#include <math.h>

#define DEBUG 1  //Debug Code  1=on 0=off.

#define DEG_TO_RAD (M_PI / 180.0)

#define bitSet(a,b) ((a) |= (1<<(b)))
#define bitClear(a,b) ((a) &= ~(1<<(b)))


// map
unsigned x,y;
const int points=40;
//Set up the world. It is only 40 units by 40 units.
unsigned mapArray[40][40];
//byte mapArray[150][150];

unsigned flags; //this a flag varable. Bit 1 is for run the rerouting.
//                     Bit 2 is used in sensorSweep() what way to sweep.

//Initial Starting point for Robot. is 10,10
int currentX=39,currentY=39;


void updatemap(int angle, int distance){
  int newX,newY,countdown;

  //Take distance in cm and convert to Feet
  distance = distance / 30;

  //Convert angle from degreese to radians
  newX = currentX + (cos(angle * DEG_TO_RAD) * distance);
  newY = currentY + (sin(angle * DEG_TO_RAD) * distance);

  //Mark on map where the obstacle was.
  if ((newX < points) && (newY < points) && (newX > 0) && (newY > 0)){

    if ((mapArray[newX][newY] >= 22)) {
      //Oh Crap! I have a new object in the Path data.
      mapArray[newX][newY] = 1;
      bitSet(flags, 1); // Set the Flag on Bit 1. This runs the Routing.
    }
    else if (mapArray[newX][newY] < 10) {
      ++mapArray[newX][newY]; 
    }

    //Mark area clear between me and obstacle.
    for (countdown = 0; countdown < distance - 1; countdown = countdown + 1) {
      newX = currentX + (cos(angle * DEG_TO_RAD) * countdown);
      newY = currentY + (sin(angle * DEG_TO_RAD) * countdown);
      if (mapArray[newX][newY] <= 10)
        mapArray[newX][newY] = 0;
    }
  }

  mapArray[currentX][currentY] = 11;
}

void waveFill() {
  int minVal;
  int reset_minVal = 13;  //Lowest value to go to. 1-10 = Used for Objects, 11 = Robot, 240 = goal
  int i;
  int y;
  for (i = 0; i < 239; i++)   //Scroll through find solution "i" times, IF IT LEAVES WHITE WITHOUT SOLVING INCREASE THIS
  {
    for (x = 0; x < points; x++)   //Scroll down the MAP Array
    {
      for (y = 0; y < points; y++)   //Scroll across the MAP Array
      {     
        //if Location is a clear or the Goal
        if (mapArray[x][y] == 0 || mapArray[x][y] == 11) //THEN FIND THE HIGHEST VALUE AROUND CURRENT COORDINATE
        {
          minVal = reset_minVal;  //Clear minVal data from last round
          //Right   ***************************************************************
          if (x < points - 1)//not out of boundary
            if (mapArray[x + 1][y] > minVal && mapArray[x + 1][y] < 241) 
            {   //IF TO THE RIGHTS VALUE IS GREATER THAN minVal & IS IN RANGE
              minVal = mapArray[x + 1][y];
            }
          //Left   ****************************************************************
          if (x > 0)
            if (mapArray[x - 1][y] > minVal && mapArray[x - 1][y] < 241) 
            {   //IF TO THE LEFTS VALUE IS GREATER THAN minVal & IS IN RANGE
              minVal = mapArray[x - 1][y];
            }
          //Down  *****************************************************************
          if (y < points - 1)
            if (mapArray[x][y + 1] > minVal && mapArray[x][y + 1] < 241) 
            {   //IF BELOWS VALUE IS GREATER THAN minVal & IS IN RANGE
              minVal = mapArray[x][y + 1];
            }
          //Up  *******************************************************************
          if (y > 0)
            if (mapArray[x][y - 1] > minVal && mapArray[x][y - 1] < 241) 
            {   //IF ABOVES VALUE IS GREATER THAN minVal & IS IN RANGE
              minVal = mapArray[x][y - 1];
            }

          //If Value > reset_minVal && Location is a Robot. SOLVED & EXIT
          if (minVal > reset_minVal && mapArray[x][y] == 11) 
          {
            return;
          }
          else if (minVal != reset_minVal)  //If Value doesn't equal the reset value change current coordinate with the min value found - 1
          {
            mapArray[x][y] = minVal - 1;
          }
        }
      }
    }
  }
  //0 = Clear, 1-10 = Object, 11 = Robot, 240 = Goal, Above 240 Reserved for Upgrades
  bitClear(flags, 1); // Clear the Flag on Bit 1. We have done the routing.
}

void clearMap() {
  for (x = 0; x < points; x++)   //Scroll down the MAP Array
  {
    for (y = 0; y < points; y++)   //Scroll across the MAP Array
    {
      if (mapArray[x][y] > 11 && mapArray[x][y] < 241)  //If the map Coordinate is between 11 & 240 reset to Zero
      {
        mapArray[x][y] = 0;
      }
    }
  }
}

void printMap(){
  //Print out the MAP!
  printf("\n");
  for (y = 0; y < points; y = y + 1) {
    printf("%d" ,y);
    printf("\t");
    for (x = 0; x < points; x = x + 1) {
      if (mapArray[x][y] < 10) { 
        printf(" ");
      }
      if (mapArray[x][y] < 100) { 
        printf(" ");
      }
      printf("%d", mapArray[x][y]);
      printf(" ");
    }
    printf("\n");
  }
}

int main(int argc, char **argv) {
mapArray[currentX][currentY] = 11;   //Mark current position on map with a 11 for debuging.
mapArray[1][1] = 240; //This is the GOAL.


	printMap();
	waveFill();
	printMap();
	
	return 0;
}


