#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include "meArm.h"
#include <Servo.h>
#include <Button.h>


// Software SPI (slower updates, more flexible pin options):
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);

// Hardware SPI (faster, but must use certain hardware pins):
// SCK is LCD serial clock (SCLK) - this is pin 13 on Arduino Uno
// MOSI is LCD DIN - this is pin 11 on an Arduino Uno
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
// Adafruit_PCD8544 display = Adafruit_PCD8544(5, 4, 3);
// Note with hardware SPI MISO and SS pins aren't used but will still be read
// and written to during SPI transfer.  Be careful sharing these pins!

Button button = Button(12,PULLUP);
boolean alternateControl = FALSE;

meArm arm;
int xdirPin = 0;
int ydirPin = 1;
int zdirPin = 3;

int basePin = 8;
int shoulderPin = 9;
int elbowPin = 10;
int gripperPin = 11;


float dx = 0.000;
float dy = 0.000;
float dz = 0.000;



void setup()   {
  Serial.begin(9600);

  arm.begin(basePin, shoulderPin, elbowPin, gripperPin);

  display.begin();
  display.setContrast(50);
  pinMode(13,OUTPUT); //debug to led 13
}

void loop() {
    if(button.isPressed()) alternateControl = !alternateControl;
    


  if (!alternateControl){
    digitalWrite(13,LOW);
  dx = map(analogRead(xdirPin), 0, 1023, -5.0, 5.0);
  dy = map(analogRead(ydirPin), 0, 1023, 5.0, -5.0);
  }
  else {
  digitalWrite(13,HIGH);
  dz = map(analogRead(xdirPin), 0, 1023, 5.0, -5.0);
  float dg = map(analogRead(ydirPin), 0, 1023, 5.0, -5.0);
  }



  if (abs(dx) < 1.5) dx = 0;
  if (abs(dy) < 1.5) dy = 0;
  if (abs(dz) < 1.5) dz = 0;
  
  if (!(dx == 0 && dy == 0 && dz == 0))
    arm.goDirectlyTo(arm.getX() + dx, arm.getY() + dy, arm.getZ() + dz);

  if (dg < -3.0)
    arm.closeGripper();
  else if (dg > 3.0)
    arm.openGripper();  

  updatedisplay();
}

void updatedisplay(){

  display.setTextColor(BLACK);
  display.clearDisplay();   // clears the screen and buffer  
  display.setCursor(0,0);
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.print("X:");
  display.setTextColor(BLACK, WHITE);
  display.println(arm.getX());
  display.setTextColor(WHITE, BLACK);
  display.print("Y:");
  display.setTextColor(BLACK, WHITE);
  display.println(arm.getY());
  display.setTextColor(WHITE, BLACK);
  display.print("Z:");
  display.setTextColor(BLACK, WHITE);
  display.println(arm.getZ());
  display.display();
}

