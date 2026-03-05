#include <Arduino.h>
#include <hmi.h>
#include <info.h>
#include <mux.h>
#include <out.h>
#include <wiring.h>

void Hmi::wait_for_booting(Sys &sys)
{
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
					sys.errors[2] = false; // clear HMI error
					cmd = "";
					return;
				}
				else
				{
					// HMI was started before MCU.
					Serial.print("t0.txt=\"");
					Serial.print("STM32\x86\xA2\x84\xD3\xD6\xD0...");	// STM32啟動中...
					Serial.print("\"\xff\xff\xff");
					Serial.print("t0.pco=63488\xff\xff\xff"); // Red color
					delay(500);
					sys.errors[2] = false; // clear HMI error
					cmd = "";
					return;
				}
			}
			else
			{
				cmd += String(new_cmd);
			}
		}
	}
}

void Hmi::update(Out &out, Sys &sys)
{
	if(Serial.available())
	{
		char new_cmd = (char)Serial.read();
		if(new_cmd == '\n')
		{
			// do nothing
		}
		else
		{
			cmd += String(new_cmd);
			return;
		}
	}
	else
		return;

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

	cmd = "";
}

