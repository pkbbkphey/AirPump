#include <Arduino.h>

#include <hmi.h>
#include <info.h>
#include <mux.h>
#include <out.h>
#include <sys.h>
#include <wiring.h>
// #include <FastLED.h>
#include <Servo.h>

Hmi hmi;
Mux mux;
Out out;
Sys sys;
Servo ServoL;
Servo ServoR;

// #define NUM_LEDS 2
// CRGB leds[NUM_LEDS];

void setup() {
	Serial.begin(115200);
	pinMode(PIN_PumpRelay1, OUTPUT);
	pinMode(PIN_PumpRelay2, OUTPUT);
	pinMode(PIN_PumpRelay3, OUTPUT);
	pinMode(PIN_PumpRelay4, OUTPUT);
	pinMode(PIN_PumpRelay5, OUTPUT);
	pinMode(PIN_PumpRelay6, OUTPUT);
	pinMode(PIN_Servo1, OUTPUT);
	pinMode(PIN_Servo2, OUTPUT);
	pinMode(PIN_FanPWM, OUTPUT);
	pinMode(PIN_MUX_S0, OUTPUT);
	pinMode(PIN_MUX_S1, OUTPUT);
	pinMode(PIN_MUX_S2, OUTPUT);
	pinMode(PIN_MUX_S3, OUTPUT);
	pinMode(PIN_MUX1_SIG, INPUT);
	pinMode(PIN_MUX2_SIG, INPUT);
	pinMode(PIN_WLED, OUTPUT);
	pinMode(PIN_LED, OUTPUT);
	// FanPWM.attach(PIN_FanPWM);
	// FanPWM.write(90);
	analogWrite(PIN_FanPWM, 0);
	ServoL.attach(PIN_Servo1);
	ServoR.attach(PIN_Servo2);

    digitalWrite(PIN_MUX_S0, LOW);
    digitalWrite(PIN_MUX_S1, LOW);
    digitalWrite(PIN_MUX_S2, LOW);
    digitalWrite(PIN_MUX_S3, LOW);

	// FastLED.addLeds<NEOPIXEL, PIN_WLED>(leds, NUM_LEDS);

	sys.errors[2] = true; // set HMI error until communication established
	
	hmi.wait_for_booting(sys);
}

void loop()
{
	mux.update(out, sys);
	// mux.debug();
	hmi.update(out, sys);
	// leds[0] = (millis() % 1000 < 500) ? CRGB::Red : CRGB::Black;
	// leds[1] = (millis() % 1000 < 500) ? CRGB::Green : CRGB::Black;
	// FastLED.show();
	
	// Determine the pump level based on input signals and settings
	// 0:L,   1:CNC,  2:R,    3:RF,   4:MANUAL
	// 0~100, 1~6, 	  0~100,  0~100,  1~6
	int l_level = 0;
	l_level = double(out.input_signal.level[0]) / 14.3;
	l_level = ((!out.setting.cnc_binded) 	&& (out.input_signal.level[1] > l_level) && (out.input_signal.state & 0b00010) >> 1) 	? out.input_signal.level[1] : l_level;
	l_level = ((!out.setting.rf_binded) 	&& (out.input_signal.level[3] / 14.3  > l_level)) 										? out.input_signal.level[3] : l_level;
	l_level = ((!out.setting.manual_binded) && (out.input_signal.level[4] > l_level) && (out.input_signal.state & 0b10000) >> 4) 	? out.input_signal.level[4] : l_level;
	int r_level = 0;
	r_level = double(out.input_signal.level[2]) / 14.3;
	r_level = ((out.setting.cnc_binded) 	&& (out.input_signal.level[1] > r_level) && (out.input_signal.state & 0b00010) >> 1) 	? out.input_signal.level[1] : r_level;
	r_level = ((out.setting.rf_binded) 		&& (out.input_signal.level[3] / 14.3 > r_level)) 										? out.input_signal.level[3] : r_level;
	r_level = ((out.setting.manual_binded) 	&& (out.input_signal.level[4] > r_level) && (out.input_signal.state & 0b10000) >> 4) 	? out.input_signal.level[4] : r_level;
	out.pump.level_target = constrain(l_level + r_level, 0, 6);

	// Update panel status
	// out.input_signal.state = (out.input_signal.level[0] > 0);
	// out.input_signal.state += (out.input_signal.level[1] > 15) << 1;
	// out.input_signal.state += (out.input_signal.level[2] > 15) << 2;
	// out.input_signal.state += (out.input_signal.level[3] > 0) << 3;
	// out.input_signal.state += (out.input_signal.level[4] > 15) << 4;

	out.input_signal.selected = (out.input_signal.level[0] / 14.3 >= l_level) && (l_level > 0);
	out.input_signal.selected += out.setting.cnc_binded ? 
								(((out.input_signal.state & 0b00010) && (out.input_signal.level[1] >= r_level) && (r_level > 0)) << 1) :
								(((out.input_signal.state & 0b00010) && (out.input_signal.level[1] >= l_level) && (l_level > 0)) << 1);
								// True iff is a leader in its own track (L or R track, depending on cnc_binded setting)
	out.input_signal.selected += ((out.input_signal.level[2] / 14.3 >= r_level) && (r_level > 0)) << 2;
	out.input_signal.selected += out.setting.rf_binded ?
								(((out.input_signal.state & 0b00100) && (out.input_signal.level[3] / 14.3 >= r_level) && (r_level > 0)) << 3) :
								(((out.input_signal.state & 0b00100) && (out.input_signal.level[3] / 14.3 >= l_level) && (l_level > 0)) << 3);
	out.input_signal.selected += out.setting.manual_binded ?
								(((out.input_signal.state & 0b10000) && (out.input_signal.level[4] >= r_level) && (r_level > 0)) << 4) :
								(((out.input_signal.state & 0b10000) && (out.input_signal.level[4] >= l_level) && (l_level > 0)) << 4);


	// =============== Pumps manager ===================
	// Calculate individual pump runtime
	uint32_t pump_rearrange_dt = millis() - out.pump.last_pump_rearrange_time;
	for(int i = 0; i < 6; ++ i)
	{
		if(out.pump.running & (1 << i))
		{
			out.pump.runtime[i] = out.pump.last_pump_rearrange_runtime[i] + pump_rearrange_dt;
		}
	}

	// Pumps rearranging
	if((pump_rearrange_dt >= out.pump.pump_rearrange_time) ||
		(out.pump.level_target != out.pump.level_target_r) ||
		sys.errors[0])
	{
		uint32_t _now = millis();
		out.pump.last_pump_rearrange_time = _now;
		// out.pump.last_pump_rearrange_time_ind[0] = now;
		// out.pump.last_pump_rearrange_time_ind[1] = now;
		// out.pump.last_pump_rearrange_time_ind[2] = now;
		// out.pump.last_pump_rearrange_time_ind[3] = now;
		// out.pump.last_pump_rearrange_time_ind[4] = now;
		// out.pump.last_pump_rearrange_time_ind[5] = now;

		// Push the available (not overtemp) pumps into a map
		std::vector<std::pair<int, uint32_t>> pumpId_runtime;
		for(int i = 0; i < 6; ++i)
		{
			if(out.pump.temperature[i] < out.setting.pump_overheat_protect)
				pumpId_runtime.push_back(std::make_pair(i, out.pump.runtime[i]));
		}
		
		// Sort ascending by the SECOND element
		std::sort(pumpId_runtime.begin(), pumpId_runtime.end(), [](const auto& a, const auto& b) {
		return a.second < b.second; 
		});

		out.pump.running = 0b000000;
		out.pump.level_actual = min(out.pump.level_target, int(pumpId_runtime.size()));
		for(int k = 0; k < out.pump.level_actual; ++ k)
		{
			out.pump.running |= (1 << pumpId_runtime[k].first);
			out.pump.last_pump_rearrange_runtime[pumpId_runtime[k].first] = out.pump.runtime[pumpId_runtime[k].first];
		}

		out.pump.level_target_r = out.pump.level_target;
	}

	// Write the results to pump relays
	digitalWrite(PIN_PumpRelay1, out.pump.running & 0b000001);
	digitalWrite(PIN_PumpRelay2, out.pump.running & 0b000010);
	digitalWrite(PIN_PumpRelay3, out.pump.running & 0b000100);
	digitalWrite(PIN_PumpRelay4, out.pump.running & 0b001000);
	digitalWrite(PIN_PumpRelay5, out.pump.running & 0b010000);
	digitalWrite(PIN_PumpRelay6, out.pump.running & 0b100000);

	
	// =============== Valve manager ================
	// Determine the left and right valves percentages based on the individual target levels
	if(out.pump.level_target != 0)	// Laziness logic: don't move the servo until it is required to.
	{
		out.valve.l_percent = (l_level >= r_level) ? 100 : (double(l_level) / double(r_level)) * 100.0;
		out.valve.r_percent = (r_level >= l_level) ? 100 : (double(r_level) / double(l_level)) * 100.0;

		if(out.valve.l_percent_r != out.valve.l_percent)
		{
			out.valve.l_last_moving_time = millis();
			out.valve.l_percent_r = out.valve.l_percent;
		}
		if(out.valve.r_percent_r != out.valve.r_percent)
		{
			out.valve.r_last_moving_time = millis();
			out.valve.r_percent_r = out.valve.r_percent;
		}
	}

	// Write the results to servos
	if((millis() - out.valve.l_last_moving_time) <= out.valve.active_time)
	{
		ServoL.attach(PIN_Servo1);
		ServoL.write(map(out.valve.l_percent, 0, 100, 147, 41));
	}
	else
		ServoL.detach();

	if((millis() - out.valve.r_last_moving_time) <= out.valve.active_time)
	{
		ServoR.attach(PIN_Servo2);
		ServoR.write(map(out.valve.r_percent, 0, 100, 145, 35));
	}
	else
		ServoR.detach();

}

