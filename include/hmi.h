#ifndef __HMI_H__
#define __HMI_H__

#include <Arduino.h>
#include <out.h>
#include <sys.h>

class Hmi
{
public:
    Hmi() {};
    void wait_for_booting(Sys &sys);
    void update(Out &out, Sys &sys);
private:
    String cmd = "";
};

#endif