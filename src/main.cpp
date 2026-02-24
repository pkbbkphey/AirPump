#include <Arduino.h>

const String MCU_VERSION = "v1.0";
const String MCU_BUILD_DATE ="2026 / 02 / 01";

#include <wiring.h>

// =================   Communication Protocol & Related Variables   ================

// HMI => MCU
/*
 *	Format 1: C<CMD_ID>\n				// Simple command or request
 *	CMD_ID  Meaning
 *  ======  =======
 *	  0     Starting page, check MCU is on.
 *	  1     Home page, refresh request.
 *    2     Status page, refresh request.
 *    3     Settings1 page, refresh request.
 *    4     Settings2 page, refresh request.
 *    5     Settings3 page, refresh request.
 *
 *  Format 2: W<VAR_ID>,<VALUE>\n		// Write variable
 *  VAR_ID  Meaning
 *  ======  =======
 *    0     backlight_autobrightness 最佳化亮度
 *    1     rf_binded RF輸出綁定
 *    2     cnc_binded CNC輸出綁定
 *    3     valve_auto_release 夾管筏自動釋放
 *    4     valve_auto_release_time 自動釋放時長
 *    5     pump_overheat_protect 氣泵過熱溫度保護
 */

// MCU => HMI
struct out
{
	String message = "一切正常";

	struct pump
	{
		int running = 0b000000;							// bit0: pump1, bit1: pump2, bit2: pump3, bit3: pump4, bit4: pump5, bit5: pump6
		int runtime[6] = {0, 0, 0, 0, 0, 0}; 			// runtime minutes for pumps 1 to 6
		int temperature[6] = {25, 25, 25, 25, 25, 25}; 	// temperature sensors 1 to 6
	};
    struct pump pump;

	struct output_flow
	{
		int state = 0b00;					// bit1: R, bit0: L
		int level = 1;						// 1 to 6
	};
    struct output_flow output_flow;

	struct input_signal
	{
		int selected = 0;					// {0: L, 1: CNC, 2: R, 3: RF, 4: MANUAL}
		int state = 0b00000;					// bit4: MANUAL, bit3: RF, bit2: R, bit1: CNC, bit0: L
		int level[5] = {0, 1, 0, 0, 0}; 		// levels for L, CNC, R, RF, MANUAL; 0-100, 1-6, 0-100, 0-100, 1-6
	};
    struct input_signal input_signal;

	int fan_level = 0;						// 0 to 100
	unsigned long machime_runtime = 0;		// in minutes
	bool rf_connected = false;				// nRF24l01 connection status

	struct setting
	{
		int backlight_brightness = 100;		// 螢幕亮度			; 0 to 100 
		int backlight_autobrightness = 1;	// 最佳化亮度		; 0: disable, 1: enable
		const String rf_info = "nRF24l01 | Channel = 1 | 2.4GHz";	// RF 連接資訊
		bool rf_binded = 0;					// RF輸出綁定		; 0: L, 1: R
		bool cnc_binded = 0;				// CNC輸出綁定		; 0: L, 1: CNC

		bool valve_auto_release = 0;		// 夾管筏自動釋放	; 0: disable, 1: enable
		int valve_auto_release_time = 30;	// 自動釋放時長		; in seconds
		int pump_overheat_protect = 30;		// 氣泵過熱溫度保護	; in degree Celsius

		// const String hmi_version = "2026 / 01 / 30 | v1.0";
		const String mcu_version = String(MCU_BUILD_DATE) + " | " + String(MCU_VERSION);
	};
    struct setting setting;
};
struct out out;
// =================================================================================

void hmi_update(String cmd);

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
		if(Serial.available())
		{
			String cmd = Serial.readStringUntil('\n');
			if(cmd.startsWith("C0")) {
				// Starting page, check MCU is on.
				Serial.print("page 1\xff\xff\xff");
				sys_errors[2] = false; // clear HMI error
				break;
			}
		}
	}
}

void loop() {
	Serial.print("status.t12.bco=63488\xff\xff\xff");
	mux_update();
	if(Serial.available()) {
		String cmd = Serial.readStringUntil('\n');
		hmi_update(cmd);
	}
}

int cnc_rotary_switch_state = 0b000001;
int manual_rotary_switch_state = 0b000001;
void mux_update() {
	// MUX1 update
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
			if(out.setting.backlight_autobrightness)
				out.setting.backlight_brightness = constrain(analogRead(PIN_MUX1_SIG) / 41, 1, 100);
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
				out.input_signal.state &= ~0b00010;
			else
				out.input_signal.state |= 0b00010;
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

void hmi_update(String cmd)
{
	// Process command from HMI
	if(cmd.startsWith("C")) {
		// Simple command or request
		int cmd_id = cmd.substring(1).toInt();
		switch(cmd_id) {
			case 0:
			{
				// Starting page, check MCU is on. At this point, HMI is probably restarted.
				Serial.print("page 1\xff\xff\xff");
				break;
			}
			case 1:
			{
				// Home page, refresh request.
				Serial.print("home.q0.picc=");
				Serial.print(out.output_flow.level + 6);
				Serial.print("\xff\xff\xff");

				Serial.print("home.q1.picc=");
				Serial.print(((out.output_flow.state & 0b01) ? 7 : 6));
				Serial.print("\xff\xff\xff");

				Serial.print("home.q2.picc=");
				Serial.print(((out.output_flow.state & 0b10) ? 7 : 6));
				Serial.print("\xff\xff\xff");

				Serial.print("home.q3.picc=");
				Serial.print((out.input_signal.selected == 0) ? 7 : 6);
				Serial.print("\xff\xff\xff");

				Serial.print("home.q4.picc=");
				Serial.print((out.input_signal.selected == 1) ? 7 : 6);
				Serial.print("\xff\xff\xff");

				Serial.print("home.q5.picc=");
				Serial.print((out.input_signal.selected == 2) ? 7 : 6);
				Serial.print("\xff\xff\xff");

				Serial.print("home.q6.picc=");
				Serial.print((out.input_signal.selected == 3) ? 7 : 6);
				Serial.print("\xff\xff\xff");
				break;
			}
			case 2:
				// Status page, refresh request.
				// (Send relevant status data)
				break;
			case 3:
				// Settings1 page, refresh request.
				// (Send relevant settings data)
				break;
			case 4:
				// Settings2 page, refresh request.
				// (Send relevant settings data)
				break;
			case 5:
				// Settings3 page, refresh request.
				// (Send relevant settings data)
				break;
			default:
				// Unknown command
				break;
		}
	} else if(cmd.startsWith("W")) {
		// Write variable
		int comma_index = cmd.indexOf(',');
		if(comma_index != -1) {
			int var_id = cmd.substring(1, comma_index).toInt();
			String value = cmd.substring(comma_index + 1);
			switch(var_id) {
				case 0:
					out.setting.backlight_autobrightness = value.toInt();
					break;
				case 1:
					out.setting.rf_binded = value.toInt();
					break;
				case 2:
					out.setting.cnc_binded = value.toInt();
					break;
				case 3:
					out.setting.valve_auto_release = value.toInt();
					break;
				case 4:
					out.setting.valve_auto_release_time = value.toInt();
					break;
				case 5:
					out.setting.pump_overheat_protect = value.toInt();
					break;
				default:
					// Unknown variable
					break;
			}
		}
	}
}