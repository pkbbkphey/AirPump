#include <Arduino.h>
#include <hmi.h>
#include <info.h>
#include <mux.h>
#include <out.h>
#include <sys.h>
#include <wiring.h>

int cnc_rotary_switch_state = 0b000001;
int manual_rotary_switch_state = 0b000001;
void Mux::update(Out &out, Sys &sys) {
	if((millis() - last_mux_update_time) < mux_update_interval_ms)
		return;
	last_mux_update_time = millis();
	// MUX1 update
	// Serial.print(analogRead(PIN_MUX1_SIG));
	// Serial.print(" ");
	// if(mux_channel_counter == 15) Serial.print("\n");

    switch (mux_channel_counter) {
        case 0 ... 5:
        {
            // Pump Temperature Sensors 1 to 6
            float temperature = analogRead(PIN_MUX1_SIG) * (3.3 / 4095.0) * 100.0;
            out.pump.temperature[mux_channel_counter] = static_cast<int>(temperature);
			if(mux_channel_counter == 5)
			{
				sys.errors[0] = false; // clear overtemp error
				if(out.pump.temperature[0] >= out.setting.pump_overheat_protect ||
				   out.pump.temperature[1] >= out.setting.pump_overheat_protect ||
				   out.pump.temperature[2] >= out.setting.pump_overheat_protect ||
				   out.pump.temperature[3] >= out.setting.pump_overheat_protect ||
				   out.pump.temperature[4] >= out.setting.pump_overheat_protect ||
				   out.pump.temperature[5] >= out.setting.pump_overheat_protect)
				{
					sys.errors[0] = true; // overtemp error
				}
			}
            break;
        }
        case 6:
		{
            // Ambient Light Sensor
			float backlight_autobrightness_new = 
				analogRead(PIN_MUX1_SIG) * 0.6 * (1-out.setting.backlight_autobrightness_rate) +
				out.setting.backlight_autobrightness_r * out.setting.backlight_autobrightness_rate;
			out.setting.backlight_autobrightness_r = backlight_autobrightness_new;

			out.setting.backlight_autobrightness = constrain((int)backlight_autobrightness_new, 1, 100);
            break;
		}
        case 7 ... 12:
		{
            // CNC Input Signal Level (Rotary Switch)
			bool cnc_in = digitalRead(PIN_MUX1_SIG);
			if(cnc_in)
				cnc_rotary_switch_state |= (1 << (mux_channel_counter - 7));
			else
				cnc_rotary_switch_state &= ~(1 << (mux_channel_counter - 7));
			if(mux_channel_counter == 12)
			{
				sys.errors[1] = false; // clear interface error
				if(cnc_rotary_switch_state == 0b000000)
					break;
				if(cnc_rotary_switch_state == 0b000001)
					out.input_signal.level[1] = 1;
				else if(cnc_rotary_switch_state == 0b000010)
					out.input_signal.level[1] = 2;
				else if(cnc_rotary_switch_state == 0b000100)
					out.input_signal.level[1] = 3;
				else if(cnc_rotary_switch_state == 0b001000)
					out.input_signal.level[1] = 4;
				else if(cnc_rotary_switch_state == 0b010000)
					out.input_signal.level[1] = 5;
				else if(cnc_rotary_switch_state == 0b100000)
					out.input_signal.level[1] = 6;
				else
					sys.errors[1] = true; // interface error
			}		
            break;
		}
        case 13:
		{
            // CNC Input (Optalcoupler)
			bool cnc_in = digitalRead(PIN_MUX1_SIG);
			if(cnc_in)
				out.input_signal.state |= 0b0010;
			else
				out.input_signal.state &= ~0b0010;
            break;
		}
        case 14:
		{
            // L Input Signal Level (Potentiometer)
			int l_level = map(analogRead(PIN_MUX1_SIG), 0, 1023, 0, 100);
			out.input_signal.level[0] = constrain(l_level, 0, 100);
			if(l_level < 10)
				out.input_signal.state &= ~0b0001;
			else
				out.input_signal.state |= 0b0001;
            break;
		}
        case 15:
		{
            // R Input Signal Level (Potentiometer)
			int r_level = map(analogRead(PIN_MUX1_SIG), 0, 1023, 0, 100);
			out.input_signal.level[2] = constrain(r_level, 0, 100);
			if(r_level < 10)
				out.input_signal.state &= ~0b00100;
			else
				out.input_signal.state |= 0b00100;
            break;
		}
    }

	// MUX2 update
	switch (mux_channel_counter) {
        case 7 ... 12:
		{
            // MANUAL Input Signal Level (Rotary Switch)
			bool manual_in = digitalRead(PIN_MUX2_SIG);
			if(manual_in)
				manual_rotary_switch_state |= (1 << (mux_channel_counter - 7));
			else
				manual_rotary_switch_state &= ~(1 << (mux_channel_counter - 7));
			if(mux_channel_counter == 12)
			{
				// sys_errors[1] = false; // clear interface error (cleared earlier)
				if(manual_rotary_switch_state == 0b000000)
					break;
				if(manual_rotary_switch_state == 0b000001)
					out.input_signal.level[4] = 1;
				else if(manual_rotary_switch_state == 0b000010)
					out.input_signal.level[4] = 2;
				else if(manual_rotary_switch_state == 0b000100)
					out.input_signal.level[4] = 3;
				else if(manual_rotary_switch_state == 0b001000)
					out.input_signal.level[4] = 4;
				else if(manual_rotary_switch_state == 0b010000)
					out.input_signal.level[4] = 5;
				else if(manual_rotary_switch_state == 0b100000)
					out.input_signal.level[4] = 6;
				else
					sys.errors[1] = true; // interface error
			}		
            break;
		}
        case 13:
		{
            // MANUAL Input (Switch)
			bool manual_in = digitalRead(PIN_MUX2_SIG);
			if(manual_in)
				out.input_signal.state |= 0b10000;
			else
				out.input_signal.state &= ~0b10000;
            break;
		}
	}
	
    mux_channel_counter = (mux_channel_counter + 1) % 16;
    digitalWrite(PIN_MUX_S0, (mux_channel_counter >> 0) & 0x01);
    digitalWrite(PIN_MUX_S1, (mux_channel_counter >> 1) & 0x01);
    digitalWrite(PIN_MUX_S2, (mux_channel_counter >> 2) & 0x01);
    digitalWrite(PIN_MUX_S3, (mux_channel_counter >> 3) & 0x01);
}