#include <Arduino.h>

const String MCU_VERSION = "v1.1";
const String MCU_BUILD_DATE ="2026 / 03 / 01";

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
		int level = 0;						// 0 to 6
	};
    struct output_flow output_flow;

	struct input_signal
	{
		int selected = 0;					// {0: L, 1: CNC, 2: R, 3: RF, 4: MANUAL}
		int state = 0b00000;					// bit4: MANUAL, bit3: RF, bit2: R, bit1: CNC, bit0: L
		int level[5] = {0, 1, 0, 0, 1}; 		// levels for L, CNC, R, RF, MANUAL; 0-100, 1-6, 0-100, 0-100, 1-6
	};
    struct input_signal input_signal;

	int fan_level = 0;						// 0 to 100
	unsigned long machime_runtime = 0;		// in minutes
	bool rf_connected = false;				// nRF24l01 connection status

	struct setting
	{
		int backlight_autobrightness = 100;		// 最佳化亮度值			; 0 to 100 
		int backlight_autobrightness_enabled = 1;	// 最佳化亮度		; 0: disable, 1: enable
		const String rf_info = "nRF24l01 | Channel = 1 | 2.4 GHz";	// RF 連接資訊
		bool rf_binded = 1;					// RF輸出綁定		; 0: L, 1: R
		bool cnc_binded = 0;				// CNC輸出綁定		; 0: L, 1: R
		bool manual_binded = 1;				// MANUAL輸出綁定	; 0: L, 1: R

		bool valve_auto_release = 0;		// 夾管筏自動釋放	; 0: disable, 1: enable
		int valve_auto_release_time = 30;	// 自動釋放時長		; in seconds
		int pump_overheat_protect = 30;		// 氣泵過熱溫度保護	; in degree Celsius

		// const String hmi_version = "2026 / 01 / 30 | v1.0";
		const String mcu_version = String(MCU_BUILD_DATE) + "  |  " + String(MCU_VERSION);
	};
    struct setting setting;
};
struct out out;
// =================================================================================

String cmd = "";
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
			out.setting.backlight_autobrightness = constrain(analogRead(PIN_MUX1_SIG) / 41, 1, 100);
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
				Serial.print((out.input_signal.selected == 0) ? 8 : ((out.input_signal.state & 0b00001) ? 7 : 6));
				Serial.print("\xff\xff\xff");

				Serial.print("home.q4.picc=");
				Serial.print((out.input_signal.selected == 1) ? 8 : ((out.input_signal.state & 0b00010) ? 7 : 6));
				Serial.print("\xff\xff\xff");

				Serial.print("home.q5.picc=");
				Serial.print((out.input_signal.selected == 2) ? 8 : ((out.input_signal.state & 0b00100) ? 7 : 6));
				Serial.print("\xff\xff\xff");

				Serial.print("home.q6.picc=");
				Serial.print((out.input_signal.selected == 4) ? 8 : ((out.input_signal.state & 0b10000) ? 7 : 6));
				Serial.print("\xff\xff\xff");

				Serial.print("home.q7.picc=");
				Serial.print((out.input_signal.selected == 3) ? 8 : ((out.input_signal.state & 0b01000) ? 7 : 6));
				Serial.print("\xff\xff\xff");
				break;
			}
			case 2:
			{
				// Status page, refresh request.
				Serial.print("status.t1.bco=");
				Serial.print((out.pump.running & 0b000001) ? 2016 : 38066);	// Green if running, gray if stopped
				Serial.print("\xff\xff\xff");

				Serial.print("status.t2.bco=");
				Serial.print((out.pump.running & 0b000010) ? 2016 : 38066);
				Serial.print("\xff\xff\xff");

				Serial.print("status.t3.bco=");
				Serial.print((out.pump.running & 0b000100) ? 2016 : 38066);
				Serial.print("\xff\xff\xff");

				Serial.print("status.t4.bco=");
				Serial.print((out.pump.running & 0b001000) ? 2016 : 38066);
				Serial.print("\xff\xff\xff");

				Serial.print("status.t5.bco=");
				Serial.print((out.pump.running & 0b010000) ? 2016 : 38066);
				Serial.print("\xff\xff\xff");

				Serial.print("status.t6.bco=");
				Serial.print((out.pump.running & 0b100000) ? 2016 : 38066);
				Serial.print("\xff\xff\xff");

				Serial.print("status.t7.txt=\"");
				Serial.print(out.pump.runtime[0]);
				Serial.print(" min\"\xff\xff\xff");

				Serial.print("status.t8.txt=\"");
				Serial.print(out.pump.runtime[1]);
				Serial.print(" min\"\xff\xff\xff");

				Serial.print("status.t9.txt=\"");
				Serial.print(out.pump.runtime[2]);
				Serial.print(" min\"\xff\xff\xff");

				Serial.print("status.t10.txt=\"");
				Serial.print(out.pump.runtime[3]);
				Serial.print(" min\"\xff\xff\xff");

				Serial.print("status.t11.txt=\"");
				Serial.print(out.pump.runtime[4]);
				Serial.print(" min\"\xff\xff\xff");

				Serial.print("status.t12.txt=\"");
				Serial.print(out.pump.runtime[5]);
				Serial.print(" min\"\xff\xff\xff");

				Serial.print("status.t13.txt=\"");
				Serial.print(out.pump.temperature[0]);
				Serial.print("\xA1\xE6");   // ℃
				Serial.print("\"\xff\xff\xff");
				Serial.print("status.t13.bco=");
				Serial.print((out.pump.temperature[0] >= out.setting.pump_overheat_protect) ? 63488 : 10995);	// Red if overheat, else blue.
				Serial.print("\xff\xff\xff");

				Serial.print("status.t14.txt=\"");
				Serial.print(out.pump.temperature[1]);
				Serial.print("\xA1\xE6");   // ℃
				Serial.print("\"\xff\xff\xff");
				Serial.print("status.t14.bco=");
				Serial.print((out.pump.temperature[1] >= out.setting.pump_overheat_protect) ? 63488 : 10995);
				Serial.print("\xff\xff\xff");

				Serial.print("status.t15.txt=\"");
				Serial.print(out.pump.temperature[2]);
				Serial.write("\xA1\xE6");   // ℃
				Serial.print("\"\xff\xff\xff");
				Serial.print("status.t15.bco=");
				Serial.print((out.pump.temperature[2] >= out.setting.pump_overheat_protect) ? 63488 : 10995);
				Serial.print("\xff\xff\xff");

				Serial.print("status.t16.txt=\"");
				Serial.print(out.pump.temperature[3]);
				Serial.write("\xA1\xE6");   // ℃
				Serial.print("\"\xff\xff\xff");
				Serial.print("status.t16.bco=");
				Serial.print((out.pump.temperature[3] >= out.setting.pump_overheat_protect) ? 63488 : 10995);
				Serial.print("\xff\xff\xff");

				Serial.print("status.t17.txt=\"");
				Serial.print(out.pump.temperature[4]);
				Serial.write("\xA1\xE6");   // ℃
				Serial.print("\"\xff\xff\xff");
				Serial.print("status.t17.bco=");
				Serial.print((out.pump.temperature[4] >= out.setting.pump_overheat_protect) ? 63488 : 10995);
				Serial.print("\xff\xff\xff");

				Serial.print("status.t18.txt=\"");
				Serial.print(out.pump.temperature[5]);
				Serial.write("\xA1\xE6");   // ℃
				Serial.print("\"\xff\xff\xff");
				Serial.print("status.t18.bco=");
				Serial.print((out.pump.temperature[5] >= out.setting.pump_overheat_protect) ? 63488 : 10995);
				Serial.print("\xff\xff\xff");

				Serial.print("status.t21.txt=\"");
				Serial.print((out.output_flow.state & 0b01) ? "ON" : "OFF");
				Serial.print("\"\xff\xff\xff");
				Serial.print("status.t21.pco=");
				Serial.print((out.output_flow.state & 0b01) ? 2016 : 65535);	// Green if on, white if off.
				Serial.print("\xff\xff\xff");

				Serial.print("status.t26.txt=\"");
				Serial.print((out.output_flow.state & 0b10) ? "ON" : "OFF");
				Serial.print("\"\xff\xff\xff");
				Serial.print("status.t26.pco=");
				Serial.print((out.output_flow.state & 0b10) ? 2016 : 65535);
				Serial.print("\xff\xff\xff");

				Serial.print("status.t22.txt=\"");
				Serial.print(out.input_signal.level[0]);
				Serial.print(" %\"\xff\xff\xff");

				Serial.print("status.t27.txt=\"");
				Serial.print(out.input_signal.level[2]);
				Serial.print(" %\"\xff\xff\xff");

				Serial.print("status.t23.txt=\"");
				Serial.print(out.input_signal.level[1]);
				Serial.print("\"\xff\xff\xff");

				Serial.print("status.t28.txt=\"");
				Serial.print(out.input_signal.level[4]);
				Serial.print("\"\xff\xff\xff");

				Serial.print("status.t24.txt=\"");
				Serial.print((out.setting.cnc_binded) ? "R" : "L");
				Serial.print("\"\xff\xff\xff");

				Serial.print("status.t29.txt=\"");
				Serial.print((out.setting.manual_binded) ? "R" : "L");
				Serial.print("\"\xff\xff\xff");

				Serial.print("status.t25.txt=\"");
				Serial.print((out.input_signal.state & 0b00010) ? "ON" : "OFF");
				Serial.print("\"\xff\xff\xff");

				Serial.print("status.t30.txt=\"");
				Serial.print((out.input_signal.state & 0b10000) ? "ON" : "OFF");
				Serial.print("\"\xff\xff\xff");

				Serial.print("status.t31.txt=\"");
				Serial.print((out.rf_connected) ? "\xD2\xD1\xDF\x42\xBD\xD3" : "\xCE\xB4\xDF\x42\xBD\xD3");	// 已連接 : 未連接
				Serial.print("\"\xff\xff\xff");

				Serial.print("status.t32.txt=\"");
				Serial.print(out.input_signal.level[3]);
				Serial.print(" %\"\xff\xff\xff");

				Serial.print("status.t33.txt=\"");
				Serial.print((out.setting.rf_binded) ? "R" : "L");
				Serial.print("\"\xff\xff\xff");

				Serial.print("status.t19.txt=\"");
				Serial.print(out.fan_level);
				Serial.print("\"\xff\xff\xff");

				Serial.print("status.t20.txt=\"");
				Serial.print(millis() / 60000);
				Serial.print("\"\xff\xff\xff");

				break;
			}
			case 3:
			{
				// Settings1 page, refresh request.
				if(out.setting.backlight_autobrightness_enabled)
				{
					Serial.print("settings1.t1.txt=\"");
					Serial.print(out.setting.backlight_autobrightness);
					Serial.print("\"\xff\xff\xff");
					Serial.print("settings1.h0.val=");
					Serial.print(out.setting.backlight_autobrightness);
					Serial.print("\xff\xff\xff");
				}

				Serial.print("settings1.bt0.val=");
				Serial.print(out.setting.backlight_autobrightness_enabled);
				Serial.print("\xff\xff\xff");

				Serial.print("settings1.t5.txt=\"");
				Serial.print(out.setting.rf_info);
				Serial.print("\"\xff\xff\xff");

				Serial.print("settings1.r0.val=");
				Serial.print(!out.setting.rf_binded);
				Serial.print("\xff\xff\xff");
				Serial.print("settings1.r1.val=");
				Serial.print(out.setting.rf_binded);
				Serial.print("\xff\xff\xff");
				break;
			}
			case 4:
			{
				// Settings2 page, refresh request.
				Serial.print("settings2.r0.val=");
				Serial.print(!out.setting.cnc_binded);
				Serial.print("\xff\xff\xff");
				Serial.print("settings2.r1.val=");
				Serial.print(out.setting.cnc_binded);
				Serial.print("\xff\xff\xff");

				Serial.print("settings2.r2.val=");
				Serial.print(!out.setting.manual_binded);
				Serial.print("\xff\xff\xff");
				Serial.print("settings2.r3.val=");
				Serial.print(out.setting.manual_binded);
				Serial.print("\xff\xff\xff");

				Serial.print("settings2.bt1.val=");
				Serial.print(out.setting.valve_auto_release);
				Serial.print("\xff\xff\xff");

				Serial.print("settings2.t1.txt=\"");
				Serial.print(out.setting.valve_auto_release_time);
				Serial.print("\"\xff\xff\xff");
				Serial.print("settings2.h0.val=");
				Serial.print(out.setting.valve_auto_release_time);
				Serial.print("\xff\xff\xff");

				Serial.print("settings2.t2.txt=\"");
				Serial.print(out.setting.pump_overheat_protect);
				Serial.print("\"\xff\xff\xff");
				Serial.print("settings2.h1.val=");
				Serial.print(out.setting.pump_overheat_protect);
				Serial.print("\xff\xff\xff");
				break;
			}
			case 5:
			{
				// Settings3 page, refresh request.
				Serial.print("settings3.t2.txt=\"");
				Serial.print(out.setting.mcu_version);
				Serial.print("\"\xff\xff\xff");
				break;
			}
			default:
				// Unknown command
				break;
		}

		if(out.setting.backlight_autobrightness_enabled)
		{
			Serial.print("dim=");
			Serial.print(out.setting.backlight_autobrightness);
			Serial.print("\xff\xff\xff");
		}

		Serial.print("t0.txt=\"");
		Serial.print("\xBE\xCD\xBE\x77");	// 就緒
		Serial.print("\"\xff\xff\xff");

		Serial.print("t0.pco=53213\xff\xff\xff"); // Jade green color
	}
	else if(cmd.startsWith("W")) {
		// Write variable
		int comma_index = cmd.indexOf(',');
		if(comma_index != -1) {
			int var_id = cmd.substring(1, comma_index).toInt();
			String value = cmd.substring(comma_index + 1);
			switch(var_id) {
				case 0:
					out.setting.backlight_autobrightness_enabled = value.toInt();
					break;
				case 1:
					out.setting.rf_binded = value.toInt();
					break;
				case 2:
					out.setting.cnc_binded = value.toInt();
					break;
				case 3:
					out.setting.manual_binded = value.toInt();
					break;
				case 4:
					out.setting.valve_auto_release = value.toInt();
					break;
				case 5:
					out.setting.valve_auto_release_time = value.toInt();
					break;
				case 6:
					out.setting.pump_overheat_protect = value.toInt();
					break;
				default:
					// Unknown variable
					break;
			}
		}
	}
}