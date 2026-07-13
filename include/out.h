// Main data structure for hardware status and settings

#ifndef __OUT_H__
#define __OUT_H__

#include <Arduino.h>
#include <info.h>

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

// Main data structure for hardware status and settings
struct Out
{
	// String message = "一切正常";

	struct pump
	{
		int level = 0;									// 0 to 6, the number of pumps running
		int running = 0b000000;							// bit0: pump1, bit1: pump2, bit2: pump3, bit3: pump4, bit4: pump5, bit5: pump6
		int runtime[6] = {0, 0, 0, 0, 0, 0}; 			// runtime minutes for pumps 1 to 6
		int temperature[6] = {25, 25, 25, 25, 25, 25}; 	// temperature sensors 1 to 6
	};
    struct pump pump;

	struct valve
	{
		int l_percent = 0;	// 0 to 100, 0 is fully closed, 100 is fully open
		int r_percent = 0;	// 0 to 100, 0 is fully closed, 100 is fully open
	};
	struct valve valve;

	// struct output_flow	// replaced by valve and pump
	// {
	// 	int state = 0b00;					// bit1: R, bit0: L
	// 	int level = 0;						// 0 to 6
	// };
    // struct output_flow output_flow;

	struct input_signal
	{
		// int selected = -1;					// {-1: none,0: L, 1: CNC, 2: R, 3: RF, 4: MANUAL}
		int selected = 0b00000;					// bit4: MANUAL, bit3: RF, bit2: R, bit1: CNC, bit0: L
		int state = 0b00000;					// bit4: MANUAL, bit3: RF, bit2: R, bit1: CNC, bit0: L
		int level[5] = {0, 1, 0, 0, 1}; 		// levels for L, CNC, R, RF, MANUAL; 0-100, 1-6, 0-100, 0-100, 1-6
		int cnc_rotary_switch_state = 0b000001;
		int manual_rotary_switch_state = 0b000001;
	};
    struct input_signal input_signal;

	int fan_level = 0;						// 0 to 100
	unsigned long machime_runtime = 0;		// in minutes
	bool rf_connected = false;				// nRF24l01 connection status

	struct setting
	{
		const float backlight_autobrightness_rate = 0.98;
		int backlight_autobrightness = 100;		// 最佳化亮度值			; 0 to 100
		float backlight_autobrightness_r = 100;	// 最佳化亮度值（記錄用）; 0 to 100
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

// =================================================================================


#endif