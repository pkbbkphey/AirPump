#include <Arduino.h>

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

	struct output_flow
	{
		int state = 0b00;					// bit0: L, bit1: R
		int level = 1;						// 1 to 6
	};

	struct input_signal
	{
		int selected = 0;					// {0: L, 1: CNC, 2: R, 3: RF}
		int state = 0b0000;					// bit0: L, bit1: CNC, bit2: R, bit3: RF
		int level[4] = {0, 1, 0, 0}; 		// levels for L, CNC, R, RF; 0-100, 1-6, 0-100, 0-100
	};

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

		const String hmi_version = "2026 / 01 / 30 | v1";
		const String mcu_version = "2026 / 01 / 30 | v1";
	};
};

// =================================================================================

void setup() {
	Serial.begin(115200);
	pinMode(PC13, OUTPUT);
}

void loop() {
	Serial.print("status.t12.bco=63488\xff\xff\xff");

}
