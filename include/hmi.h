#ifndef __HMI_H__
#define __HMI_H__

#include <Arduino.h>

extern String cmd;
void hmi_update(String cmd);

#endif