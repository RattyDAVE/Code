/*
  Delivery ClockTHREE world clock application
  Light up world map where sun is currently shining.

  Justin Shaw Jan 29, 2011
  
  Licenced under Creative Commons Attribution.
  Attribution 3.0

*/

#include "UserConf.h"
#ifdef WANT_APP_SUN

#include "WiseClock.h"
#include "HT1632.h"
#include "AlarmClock.h"
#include "Sound.h"
#include "AppSun.h"
#include "TimeUtils.h"


#define DEG		(PI / 180.)
#define WSOLSTICE	(-8)
#define TROPIC_LAT	(23.45 * DEG)

#define DLON		((360. * DEG) / 32.)		// lon radiuns per led row
#define DLAT		((-180. * DEG) / 16.)		// lat radians per led col
#define LON0		((-180. * DEG) + (DLON / 2.))	// lon of left most column
#define LAT0		((90. * DEG) + (DLAT / 2.))	// lat of top row


#if NUM_DISPLAYS > 1
#define X_OFFSET 32
#else
#define X_OFFSET 0
#endif


/*
 * Convert lat/lon to xyz (ECEF) coordinates 
 * lat/lon - latitude / longetude coords in radians.
 * xyz -- unit vector in ECEF coords
 */
void CAppSun::latlon2xyz(double lat, double lon, double *xyz)
{
  xyz[0] = cos(lat) * cos(lon);
  xyz[1] = cos(lat) * sin(lon);
  xyz[2] = sin(lat);
}


/*
 * Compute sun vector in Earth Centered Earth Fixed (ECEF)coordinates
 */
void CAppSun::sun_pointing(int day, int minute, double *earth_to_sun)
{
  double lat, lon, theta;

  lon = PI - minute / 1440. * 2 * PI;
  theta = ((day - WSOLSTICE) / 365.25) * 2 * PI;
  lat = -TROPIC_LAT * cos(theta);
  latlon2xyz(lat, lon, earth_to_sun);
}


double CAppSun::dot(double *v0, double *v1, int n)
{
  double out = 0.;
  for(int i = 0; i < n; i++){
    out += v0[i] * v1[i];
  }
  return out;
}


/*
 * Compute elevation angle of the sun.  
 * lat/lon - latitude / longetude coords in radians
 */
double CAppSun::sun_el(double lat, double lon, double *sunv)
{
  double xyz[3];
  latlon2xyz(lat, lon, xyz);
  return HALF_PI - acos(dot(xyz, sunv, 3));
}


uint8_t CAppSun::getColor(double el)
{
  uint8_t color = BLACK;
  
  if(el > 15 * DEG){
    color = GREEN;
  }
  else if(el > 5 * DEG){
    color = ORANGE;
  }
  else if(el > -5 * DEG){
    color = RED;
  }
  return color;
}


char CAppSun::setPixel(coord_t column, int8_t row, double *sunv)
{
  double lat, lon, el;
  uint8_t color;
  char out;

  lat = LAT0 + row * DLAT;
  lon = LON0 + column * DLON;
  el = sun_el(lat, lon, sunv);

  color = getColor(el);
  ht1632_plot(column + X_OFFSET, row, color);
  if(el > 0){
    out = 'G';
  } else{
    out = ' ';
  }
  return out;
}


void CAppSun::init(byte mode /*=0*/)
{
  last_min = 100;
  blinkFlg = 0;
}


int16_t CAppSun::run()
{
  double sunv[3];
  int day, hr, min;

#ifdef WANT_TIME_AT_CITY
 #if NUM_DISPLAYS > 1
  if (blinkFlg > 1) {
    if (!wiseClock.isLineInProgress()) {		// line ended, restore point color
      ht1632_plot(blinkX, blinkY, blinkCol);
      blinkFlg = 0;
    } else if ((long)(millis() - blinkTim) >= 0) {
      if (blinkFlg > 2) {
	ht1632_plot(blinkX, blinkY, ORANGE);
	blinkFlg = 2;
      } else {
	ht1632_plot(blinkX, blinkY, BLACK);
	blinkFlg = 3;
      }
      blinkTim = millis() + 250;
    }
  }
 #endif
#endif
  if (
     (alarmClock.minute == last_min)
  ||  alarmClock.isTimeSetting
  ||  alarmClock.isDateSetting
  ) {
    return(50);		// nothing to do yet
  }
  last_min = alarmClock.minute;

//  day = wiseClock.julian_day(alarmClock.year, alarmClock.month, alarmClock.day);
  day = julian_day(alarmClock.year, alarmClock.month, alarmClock.day);

  hr = alarmClock.hour - wiseClock.utcDiff;
  if (wiseClock.DSTactive) hr--;
  if (hr < 0) {
    hr += 24;		// advance by one day
    day--;		// decrement day by one
    if (day < 0) day += 365;
  } else if (hr > 23) {
    hr -= 24;		// decrement by one day
    day++;		// increment day by one
  }
  min = (hr * 60) + alarmClock.minute;

  sun_pointing(day, min, sunv);

  for(int c = 0; c < 32; c++){
    unsigned long ms;
    for(int r = 0; r < 16; r++){
      setPixel(c, r, sunv);
      ms = millis();
      wiseClock.checkScroll(ms);
    }
    checkSound(ms);
    wiseClock.checkSerial();
  }
  if (blinkFlg > 0)
    blinkCol = get_shadowram(blinkX, blinkY);
  return(50);
}


// blink the specified latitude, longitute on the display.
void CAppSun::blinkLatLon(int16_t lat, int16_t lon)
{
#ifdef WANT_TIME_AT_CITY
 #if NUM_DISPLAYS > 1
  if (blinkFlg > 1)
    ht1632_plot(blinkX, blinkY, blinkCol);

  blinkY = (360 - (lat * 4)) / 45;
  if (blinkY < 0) blinkY = 0;
  else if (blinkY > 15) blinkY = 15;

  blinkX = (720 + (lon * 4)) / 45;
  if (blinkX < 0) blinkX = 0;
  else if (blinkX > 31) blinkX = 31;
  blinkX += X_OFFSET;

  blinkCol = get_shadowram(blinkX, blinkY);
  blinkFlg = 2;
  ht1632_plot(blinkX, blinkY, ORANGE);
  blinkTim = millis() + 250;
 #endif
#endif
}


CAppSun appSun;

#endif
