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
Servo FanPWM;

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
	int l_level = 0;
	l_level = out.input_signal.level[0];
	l_level = ((!out.setting.cnc_binded) 	&& (out.input_signal.level[1] > l_level)) ? out.input_signal.level[1] : l_level;
	l_level = ((!out.setting.rf_binded) 	&& (out.input_signal.level[3] > l_level)) ? out.input_signal.level[3] : l_level;
	l_level = ((!out.setting.manual_binded) && (out.input_signal.level[4] > l_level)) ? out.input_signal.level[4] : l_level;
	int r_level = 0;
	r_level = out.input_signal.level[2];
	r_level = ((!out.setting.cnc_binded) 	&& (out.input_signal.level[1] > r_level)) ? out.input_signal.level[1] : r_level;
	r_level = ((!out.setting.rf_binded) 	&& (out.input_signal.level[3] > r_level)) ? out.input_signal.level[3] : r_level;
	r_level = ((!out.setting.manual_binded) && (out.input_signal.level[4] > r_level)) ? out.input_signal.level[4] : r_level;
	out.pump.level = constrain(l_level + r_level, 0, 6);

	// Determine the left and right valves percentages based on the individual target levels
	out.valve.l_percent = (l_level >= r_level) ? 100 : (double(l_level) / double(r_level)) * 100.0;
	out.valve.r_percent = (r_level >= l_level) ? 100 : (double(r_level) / double(l_level)) * 100.0;
}

