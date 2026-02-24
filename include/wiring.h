#ifndef __WIRING_H__
#define __WIRING_H__
#include <Arduino.h>

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
constexpr int PIN_MUX1_SIG = PB0; // ADC
constexpr int PIN_MUX2_SIG = PB1; // ADC
/*
 *	MUX1 Channel Mapping:
 *	Channel  Signal
 *	=======  ======
 *	 0 ~ 5	 Pump Temperature Sensors 1 to 6
 *	 6		 Ambient Light Sensor
 *	 7 ~ 12  CNC Input Signal Level (Rotary Switch)
 *	 13      CNC Input (Optalcoupler)
 *   14      L Input Signal Level (Potentiometer)
 *   15      R Input Signal Level (Potentiometer)
 * 
 *  MUX2 Channel Mapping:
 *  Channel  Signal
 *  =======  ======
 *   7 ~ 12  Manual Input Signal Level (Rotary Switch)
 *   13      Manual Input (Switch)
 */

constexpr int PIN_BUZZER = PB9; // Timer
constexpr int PIN_WLED = PB10;	// Timer, 5V Tolerant
constexpr int PIN_LED = PC13;	// Onboard Status LED

/*
 *	Other Pins Used:
 *	Pin		Function
 *	===		========
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

#endif