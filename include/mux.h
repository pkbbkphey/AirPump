#ifndef __MUX_H__
#define __MUX_H__

#include <Arduino.h>
#include <out.h>
#include <sys.h>

class Mux
{
public:
    Mux() {};
    void update(Out &out, Sys &sys);
    void debug();
private:
    const int mux_update_interval_ms = 4;
    unsigned long last_mux_update_time = 0;
    int mux_channel_counter = 0;
};

#endif