#ifndef __SYS_H__
#define __SYS_H__

#include <Arduino.h>

struct Sys
{
    enum State {
        STATE_IDLE,
        STATE_RUNNING,
        STATE_ERROR_OVERTEMP,
        STATE_ERROR_INTERFACE,
        STATE_ERROR_HMI
    };
    State state = STATE_IDLE;
    bool errors[3] = {false, false, false};	// overtemp, interface, hmi
};

#endif