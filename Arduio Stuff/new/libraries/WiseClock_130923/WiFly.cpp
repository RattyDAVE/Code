// WiFly.cpp

//*************************************************************************
//*	Edit History, started September, 2012
//*	please put your initials and comments here anytime you make changes
//*************************************************************************
//* Sep 01/12 (mcm)  initial version
//* Jan 26/13 (mcm)  correct buffer offsets in WIFLY_TIME_SET
//*************************************************************************

#include "UserConf.h"
#ifdef WANT_WIFLY

#include "WiFly.h"
#include "WiseClock.h"
#include "AlarmClock.h"
#include "SdFatUtil.h"

//#define WiFly_DBG		// uncomment for debugging messages on serial

enum				// WiFly module states
{
    WIFLY_NOOP = 0,		// no operation
    WIFLY_CMD_START,		// send '$$$' to put into command mode
    WIFLY_CMD_CHECK,		// make sure are in command mode
    WIFLY_CMD_EXIT,		// leave command mode
    WIFLY_SET_DEVID,		// set device ID
    WIFLY_SET_SSID,		// set wlan SSID
    WIFLY_SET_PHRASE,		// set wlan pass phrase/key
    WIFLY_SET_CHAN,		// set wlan channel to 0 (search)
    WIFLY_SET_SNTP,		// set SNTP server ip address
    WIFLY_SET_PASSWD,		// set tcp open password
    WIFLY_SET_PRINTLVL,		// set diagnostic print level to 0
    WIFLY_SET_JOIN,		// set wlan join to initiate connection
    WIFLY_TIME,			// initiate sNTP time sync
    WIFLY_SHOW_NET,		// get time sync status
    WIFLY_SHOW_TIME,		// get the time
    WIFLY_TIME_SET,		// set wiseclock minutes & seconds to sNTP time
    WIFLY_GET_IP,		// get WiFly module's IP address for STATUS app
    WIFLY_GET_MAC,		// get WiFly module's MAC address for STATUS app
    WIFLY_CLOSE,		// close TCP connection
};

//
// initialize the serial interface to the WiFly module
//
void CWiFly::init()
{
    Serial1.begin(9600);
    configInit();
#ifdef WiFly_DBG
    Serial.begin(9600);
#endif
}

//
// initialize WiFly configuration information
//
void CWiFly::configInit()
{
    cmdActive = false;
    timerActive = false;
    state = WIFLY_NOOP;
    mPos = 0;
    msgBuf[0] = 0;
    addr[0] = 0;
    mac[0] = 0;
    time[0] = 0;
    sntp[0] = 0;
    passwd[0] = 0;
    deviceId[0] = 0;
    tsMinute = random(5, 55);
    tsSecond = random(5, 55);
}

//
// parse a line from the 'messages.txt' file, looking for 'WiFly.XXX' config
// commands.
// fill in the appropriate buffer with the config information.
//
void CWiFly::parseConfig(char *ln)
{
    char *s;
    if (strncasecmp_P(ln, PSTR("WiFly."), 6) != 0)
	return;
    ln += 6;
    if (confMatch(ln, PSTR("ssid"), 4, ssid, 32)) {
	// replace any blanks in the ssid with '$' -- WiFly module converts it back
	for (s = ssid; *s != 0; s++) {
	    if (*s == ' ')
		*s = '$';
	}
	return;
    }
    if (confMatch(ln, PSTR("phrase"), 6, phrase, 64)) {
	// replace any blanks in the phrase with '$' -- WiFly module converts it back
	for (s = ssid; *s != 0; s++) {
	    if (*s == ' ')
		*s = '$';
	}
	doWep = false;
	return;
    }
    if (confMatch(ln, PSTR("key"), 3, phrase, 64)) {
	doWep = true;
	return;
    }
    if (confMatch(ln, PSTR("sntp"), 4, sntp, 15))
	return;
    if (confMatch(ln, PSTR("openpwd"), 7, passwd, 20))
	return;
    if (confMatch(ln, PSTR("deviceid"), 8, deviceId, 32))
	return;
}

//
// check if the first 'ml' characters of the string '*s' matches the string '*m'
// ignoring case.  If so, skip the following white-space characters, then
// copy the remainder of the line, up to 'dl' characters, into the buffer 'd'.
//
// The match string '*m' is in PROGMEM.
//
boolean CWiFly::confMatch(char *s, PGM_P m, uint8_t ml, char *d, uint8_t dl)
{
    uint8_t i;

    if (strncasecmp_P(s, m, ml) != 0) {
	return false;
    }

    s += ml;
    while(*s == ' ' || *s == '\t')	// skip white space
	s++;

    if (dl != 0) {
	i = strlen(s);
	if (i > dl)
	    i = dl;
	memmove(d, s, i+1);			// copy null at end
	if (wiseClock.isShortTrunc && (i < dl))
	    wiseClock.readLineFromShortSD(&d[i], dl - i);
	d[dl] = 0;
    }
    return true;
}

void CWiFly::check()
{
    uint8_t c;
    while(Serial1.available() > 0) {
	c = Serial1.read();
#ifdef WiFly_DBG
	Serial.write(c);
#endif
	if (c != '\r' && c != '\n') {
	    if (mPos < (sizeof(msgBuf) - 2)) {
		msgBuf[mPos++] = c;
		msgBuf[mPos] = 0;
	    }
	} else {
	    mPos = 0;
	    if (confMatch(msgBuf, PSTR("CMD"), 3, 0, 0)) {
		if (msgBuf[3] == 0)
		    cmdActive = true;
	    } else if (confMatch(msgBuf, PSTR("*OPEN*"), 6, 0, 0)) {
		strcpy(&wiseClock.personalMsg[1], &msgBuf[6]);
#ifdef SERIAL_MSG
		wiseClock.checkMessageCmd = true;
#endif
		opnActive = true;
		if (state == WIFLY_NOOP) {
		    state = WIFLY_CMD_START;
		    nextState = WIFLY_CLOSE;	// close the tcp connection
		    timer = millis() + 300;
		    timerActive = 1;
		}
	    } else if (confMatch(msgBuf, PSTR("*CLOS*"), 6, 0, 0)) {
		if (msgBuf[6] == 0)
		    opnActive = false;
	    } else if (cmdActive) {
		if (confMatch(msgBuf, PSTR("EXIT"), 4, 0, 0)) {
		    if (msgBuf[4] == 0)
			cmdActive = false;
		} else if (confMatch(msgBuf, PSTR("IP="), 3, addr, 15)) {
		    char *s;
		    for(s = addr; *s; s++) {
			if (*s == ':') *s = 0;
		    }
		} else if (confMatch(msgBuf, PSTR("Mac Addr="), 9, mac, 17)) {
		} else if (confMatch(msgBuf, PSTR("Time="), 5, time, 8)) {
		}
	    }
	}
    }
    if (timerActive) {
	if (msgBuf[0] == '<') {		// check if CMD has completed, something like '<2.34>'
	    char *s, c;
	    s = &msgBuf[1];
	    for(c = *s; (((c >= '0') && (c <= '9')) || (c == '.')); c = *++s)
		;
	    if (c == '>') {		// command has completed, abort timer
		timer = millis() + 10;
		msgBuf[0] = 0;
		*s = 0;
		mPos = 0;
	    }
	}
	if ((long)(millis() - timer) >= 0) {
	    timerActive = false;
	    switch(state) {
	    case WIFLY_CMD_START:		// switch to command mode
		print_P(PSTR("$$$"));
		state = WIFLY_CMD_CHECK;
		timerActive = 1;
		timer = millis() + 300;
		break;

	    case WIFLY_CMD_CHECK:
		if (!cmdActive) {		// make sure command mode is enabled
		    print_P(PSTR("\rexit\r"));
		    state = WIFLY_CMD_START;
		    timer = millis() + 500;
		} else {
		    state = nextState;
		}
		timerActive = 1;
		break;

	    case WIFLY_CMD_EXIT:
		print_P(PSTR("exit\r"));
		state = WIFLY_NOOP;
		nextState = WIFLY_NOOP;
		break;

	    case WIFLY_SET_DEVID:
		if (deviceId[0] != 0) {
		    print_P(PSTR("set opt deviceid "));
		    Serial1.print(deviceId);
		    Serial1.write('\r');
		    timer = millis() + 1000;
		}
		state = WIFLY_SET_SSID;
		timerActive = 1;
		break;

	    case WIFLY_SET_SSID:
		print_P(PSTR("set wlan ssid "));
		Serial1.print(ssid);
		Serial1.write('\r');
		state = WIFLY_SET_PHRASE;
		timer = millis() + 1000;
		timerActive = 1;
		break;

	    case WIFLY_SET_PHRASE:
		if (doWep) {
		    print_P(PSTR("set wlan key "));
		} else {
		    print_P(PSTR("set wlan phrase "));
		}
		Serial1.print(phrase);
		Serial1.write('\r');
		state = WIFLY_SET_CHAN;
		timer = millis() + 1000;
		timerActive = 1;
		break;

	    case WIFLY_SET_CHAN:
		print_P(PSTR("set wlan channel 0\r"));
		state = WIFLY_SET_SNTP;
		timer = millis() + 1000;
		timerActive = 1;
		break;

	    case WIFLY_SET_SNTP:
		if (sntp[0] != 0) {
		    print_P(PSTR("set time address "));
		    Serial1.print(sntp);
		    Serial1.write('\r');
		    timer = millis() + 1000;
		}
		state = WIFLY_SET_PASSWD;
		timerActive = 1;
		break;

	    case WIFLY_SET_PASSWD:
		if (passwd[0] != 0) {
		    print_P(PSTR("set opt password "));
		    Serial1.print(passwd);
		    Serial1.write('\r');
		    timer = millis() + 1000;
		}
		state = WIFLY_SET_PRINTLVL;
		timerActive = 1;
		break;

	    case WIFLY_SET_PRINTLVL:
		print_P(PSTR("set sys printlvl 0\r"));
		state = WIFLY_SET_JOIN;
		timer = millis() + 1000;
		timerActive = 1;
		break;

	    case WIFLY_SET_JOIN:
		print_P(PSTR("set wlan join 1\r"));
		state = WIFLY_CMD_EXIT;
		timer = millis() + 1000;
		timerActive = 1;
		break;

	    case WIFLY_TIME:
		print_P(PSTR("time\r"));
		state = WIFLY_SHOW_NET;
		timer = millis() + 1000;
		timerActive = 1;
		break;

	    case WIFLY_SHOW_NET:
		print_P(PSTR("show net\r"));
		state = WIFLY_SHOW_TIME;
		timer = millis() + 1000;
		timerActive = 1;
		break;

	    case WIFLY_SHOW_TIME:
		if (time[0] == 'O' && time[1] == 'K') {
		    print_P(PSTR("show time\r"));
		    state = WIFLY_TIME_SET;
		} else {
		    state = WIFLY_CMD_EXIT;
		}
		timer = millis() + 1000;
		timerActive = 1;
		break;

	    case WIFLY_TIME_SET:
		if (time[2] == ':' && time[5] == ':') {
		    int8_t hr = atoi(&time[0]);
		    int8_t mn = atoi(&time[3]);
		    int8_t sc = atoi(&time[6]);
		    if (hr >= 0 && hr <= 23 && mn >= 0 && mn <= 59 && sc >= 0 && sc <= 59) {
			if ((mn != alarmClock.minute) || (sc != alarmClock.second)) {
			    alarmClock.setTime(alarmClock.hour, mn, sc, alarmClock.year, alarmClock.month, alarmClock.day, alarmClock.dow);
#ifdef WiFly_DBG
			    Serial.println("Setting clock to WiFly time");
#endif
			}
		    }
		}
		state = WIFLY_CMD_EXIT;
		timer = millis() + 1000;
		timerActive = 1;
		break;

	    case WIFLY_GET_IP:
		print_P(PSTR("get ip\r"));
		state = WIFLY_GET_MAC;
		timer = millis() + 1000;
		timerActive = 1;
		break;

	    case WIFLY_GET_MAC:
		print_P(PSTR("get mac\r"));
		state = WIFLY_CMD_EXIT;
		timer = millis() + 1000;
		timerActive = 1;
		break;

	    case WIFLY_CLOSE:
		print_P(PSTR("close\r"));
		state = WIFLY_CMD_EXIT;
		timer = millis() + 1000;
		timerActive = 1;
		break;
	    }
	}
    }
}

void CWiFly::config()
{
    if (ssid[0] != 0 && phrase[0] != 0) {
	state = WIFLY_CMD_START;
	nextState = WIFLY_SET_DEVID;
	timer = millis() + 300;
	timerActive = 1;
    }
}

void CWiFly::timeSet()
{
    if (state == WIFLY_NOOP && ssid[0] != 0 && phrase[0] != 0 && sntp[0] != 0) {
	state = WIFLY_CMD_START;
	nextState = WIFLY_TIME;
	timer = millis() + 300;
	timerActive = 1;
    }
    tsMinute += random(20,55);
    if (tsMinute > 59) tsMinute -= 60;
    if (tsMinute < 3) tsMinute = 3;
    tsSecond = random(15,55);
}

void CWiFly::getNet()
{
    if (state == WIFLY_NOOP) {
	state = WIFLY_CMD_START;
	nextState = WIFLY_GET_IP;
	timer = millis() + 300;
	timerActive = 1;
    }
}

void CWiFly::print_P(PGM_P str)
{
    SdFatUtil::print_P(&Serial1, str);
}

CWiFly wiFly;
#endif
