#include <Arduino.h>

#include <hmi.h>
#include <info.h>
#include <mux.h>
#include <out.h>
#include <wiring.h>

constexpr int mux_update_interval_ms = 2;
unsigned long last_mux_update_time = 0;
int mux_channel_counter = 0;
void mux_update();

enum state {
	STATE_IDLE,
	STATE_RUNNING,
	STATE_ERROR_OVERTEMP,
	STATE_ERROR_INTERFACE,
	STATE_ERROR_HMI
};
state sys_state = STATE_IDLE;
bool sys_errors[3] = {false, false, false};	// overtemp, interface, hmi

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

    digitalWrite(PIN_MUX_S0, LOW);
    digitalWrite(PIN_MUX_S1, LOW);
    digitalWrite(PIN_MUX_S2, LOW);
    digitalWrite(PIN_MUX_S3, LOW);

	sys_errors[2] = true; // set HMI error until communication established
	
	while(true)
	{
		// Serial.print("Waiting for HMI communication...\n");
		if(Serial.available())
		{
			char new_cmd = (char)Serial.read();
			// Serial.print("Received char: " + String(new_cmd) + "\n");
			if(new_cmd == '\n')
			{
				// Serial.print("Received command: " + cmd + "\n");
				if(cmd == "C0")
				{
					// HMI communication established.
					Serial.print("page 1\xff\xff\xff");
					sys_errors[2] = false; // clear HMI error
					cmd = "";
					break;
				}
				else
				{
					// HMI was started before MCU.
					Serial.print("t0.txt=\"");
					Serial.print("STM32\x86\xA2\x84\xD3\xD6\xD0...");	// STM32啟動中...
					Serial.print("\"\xff\xff\xff");
					Serial.print("t0.pco=63488\xff\xff\xff"); // Red color
					delay(500);
					sys_errors[2] = false; // clear HMI error
					cmd = "";
					break;
				}
			}
			else
			{
				cmd += String(new_cmd);
			}
		}
	}
}

void loop()
{
	mux_update();
	if(Serial.available())
	{
		char new_cmd = (char)Serial.read();
		if(new_cmd == '\n')
		{
			hmi_update(cmd);
			cmd = "";
		}
		else
		{
			cmd += String(new_cmd);
		}
	}
}

int cnc_rotary_switch_state = 0b000001;
int manual_rotary_switch_state = 0b000001;
void mux_update() {
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
				sys_errors[0] = false; // clear overtemp error
				if(out.pump.temperature[0] >= out.setting.pump_overheat_protect ||
				   out.pump.temperature[1] >= out.setting.pump_overheat_protect ||
				   out.pump.temperature[2] >= out.setting.pump_overheat_protect ||
				   out.pump.temperature[3] >= out.setting.pump_overheat_protect ||
				   out.pump.temperature[4] >= out.setting.pump_overheat_protect ||
				   out.pump.temperature[5] >= out.setting.pump_overheat_protect)
				{
					sys_errors[0] = true; // overtemp error
				}
			}
            break;
        }
        case 6:
		{
            // Ambient Light Sensor
			out.setting.backlight_autobrightness = constrain(analogRead(PIN_MUX1_SIG) * 0.6, 1, 100);
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
				sys_errors[1] = false; // clear interface error
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
					sys_errors[1] = true; // interface error
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
					sys_errors[1] = true; // interface error
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

