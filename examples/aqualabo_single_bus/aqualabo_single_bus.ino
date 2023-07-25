#include <SDI12.h>

#define SERIAL_BAUD 115200 /*Sets outout baud rate*/
#define DATA_PIN 8 /*Defines pin used for SDI-12 bus*/
#define POWER_PIN -1 /*Defines power pin (we used the 5v pin so power pin is set as -1)*/

SDI12 mySDI12(DATA_PIN);

/*Keep track of active addresses, r*/
bool isActive[64] = {
    0,
};

/*Keeps track of wait time for each active address*/
uint8_t meas_time_ms[64] = {
    0
};

/*Keep track of time each sensor was started*/
uint32_t millisStarted[64] = {
    0
};

/*Keep track of the time each sensor will be ready*/
uint32_t millisReady[64] = {
    0
};

/*Keep track of number of expected results*/
uint8_t returnedResults[64] = {
    0
};

uint8_t numSensors = 0;

/**
 * @brief converts allowable address characters ('0'-'9', 'a'-'z', 'A'-'Z') to a
 * decimal number between 0 and 61 (inclusive) to cover the 62 possible
 * addresses.
 */
byte charToDec(char i) {
  if ((i >= '0') && (i <= '9')) return i - '0';
  if ((i >= 'a') && (i <= 'z')) return i - 'a' + 10;
  if ((i >= 'A') && (i <= 'Z'))
    return i - 'A' + 36;
  else
    return i;
}

