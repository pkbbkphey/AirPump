#define MCU_VERSION "v1.0"
#define MCU_BUILD_DATE "2024 / 06 / 01"

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

		// const String hmi_version = "2026 / 01 / 30 | v1.0";
		const String mcu_version = String(MCU_BUILD_DATE) + " | " + String(MCU_VERSION);
	};
};

// =================================================================================

// =================   Wiring   ================
constexpr int PIN_PumpRelay1 = PB12;
constexpr int PIN_PumpRelay2 = PB13;
constexpr int PIN_PumpRelay3 = PB14;
constexpr int PIN_PumpRelay4 = PB15;
constexpr int PIN_PumpRelay5 = PA11;
constexpr int PIN_PumpRelay6 = PA12;

constexpr int PIN_Servo1 = PA8;
constexpr int PIN_Servo2 = PA9;
constexpr int PIN_FanPWM = PA10;

constexpr int PIN_MUX_S0 = PA15;
constexpr int PIN_MUX_S1 = PB3;
constexpr int PIN_MUX_S2 = PB4;
constexpr int PIN_MUX_S3 = PB5;
constexpr int PIN_MUX_SIG = PA1; // ADC
/*
 *	MUX Channel Mapping:
 *	Channel  Signal
 *	=======  ======
 *	 0 ~ 5	 Pump Temperature Sensors 1 to 6
 *	 6		 Ambient Light Sensor
 *	 7 ~ 12  CNC Input Signal Level (Rotary Switch)
 *	 13      CNC Input (Optalcoupler)
 *   14      L Input Signal Level (Potentiometer)
 *   15      R Input Signal Level (Potentiometer)
 */

constexpr int PIN_WLED = PB10;

/*
 *	Other Pins Used:
 *	Pin		Function
 *	===		========
 *	PC13	Onboard Status LED
 *  PA2	    Serial TX (to HMI)
 *  PA3	    Serial RX (from HMI)
 *  PA4	    nRF24L01 CSN
 *  PA5	    nRF24L01 SCK
 *  PB6     nRF24L01 MISO
 *  PB7     nRF24L01 MOSI
 *  R       Reset Button
 *  ST-link ST-link Programmer
 *  BOOT0   Boot0 Switch
 */
// ===========================================

void setup() {
	Serial.begin(115200);
	pinMode(PC13, OUTPUT);
}

void loop() {
	Serial.print("status.t12.bco=63488\xff\xff\xff");

}
