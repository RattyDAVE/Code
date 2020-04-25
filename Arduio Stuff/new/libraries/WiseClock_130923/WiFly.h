// WiFly.h

//*************************************************************************
//*	Edit History, started September, 2012
//*	please put your initials and comments here anytime you make changes
//*************************************************************************
//* Sep 01/12 (mcm)  initial version
//*************************************************************************

#ifndef _WiFly_H_
#define _WiFly_H_

#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include <avr/pgmspace.h>
#include <stdlib.h>

class CWiFly
{
public:		// public storage
    char addr[16];		// IP address of WiFly
    char mac[18];		// MAC address of WiFly
    int8_t tsMinute;		// minute at which to initiate sNTP
    int8_t tsSecond;		// second at which to initiate sNTP

private:	// private storage
    char ssid[33];		// SSID
    char phrase[65];		// WPA/WPA2 pass phrase / WEP key
    char sntp[16];		// sNTP server IP address
    char passwd[21];		// tcp open password
    char deviceId[33];		// device ID, for dhcp/dns entry
    char time[9];		// time from "show time"
    char msgBuf[128];		// message buffer
    boolean doWep;		// use WEP instead of WPA/WPA2
    boolean cmdActive;		// currently in "command" mode
    boolean opnActive;		// someone has initiated a TCP connection
    boolean timerActive;	// event timer is active
    uint32_t timer;		// timer value
    uint8_t state;		// current state
    uint8_t nextState;		// when command sequence finishes, enter this state
    uint8_t mPos;		// message position

public:		// public functions
    void init();		// initialize serial, config info
    void configInit();		// initialize just config info
    void parseConfig(char *ln);	// parse a message.txt line
    void config();		// send the config commands to the WiFly module
    void check();		// look for messages from the module
    void getNet();		// get the WiFly's MAC and IP address
    void timeSet();		// initiate an sNTP request, set the time

private:	// private functions
    boolean confMatch(char *s, PGM_P m, uint8_t ml, char *d, uint8_t dl);
    void print_P(PGM_P str);
};

extern CWiFly wiFly;

#endif	// _WiFly_H_
