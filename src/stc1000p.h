/*
 * STC1000+, improved firmware and Arduino based firmware uploader for the STC-1000 dual stage thermostat.
 *
 * Copyright 2014 Mats Staffansson
 *
 * This file is part of STC1000+.
 *
 * STC1000+ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * STC1000+ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with STC1000+.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Schematic of the connections to the MCU.
 *
 *                                     PIC16F1828
 *                                    ------------
 *                                VDD | 1     20 | VSS
 *                     Relay Heat RA5 | 2     19 | RA0/ICSPDAT (Programming header), Piezo buzzer
 *                     Relay Cool RA4 | 3     18 | RA1/AN1/ICSPCLK (Programming header), Thermistor
 * (Programming header) nMCLR/VPP/RA3 | 4     17 | RA2/AN2 Thermistor
 *                          LED 5 RC5 | 5     16 | RC0 LED 0
 *                   LED 4, BTN 4 RC4 | 6     15 | RC1 LED 1
 *                   LED 3, BTN 3 RC3 | 7     14 | RC2 LED 2
 *                   LED 6, BTN 2 RC6 | 8     13 | RB4 LED Common Anode 10's digit
 *                   LED 7, BTN 1 RC7 | 9     12 | RB5 LED Common Anode 1's digit
 *        LED Common Anode extras RB7 | 10    11 | RB6 LED Common Anode 0.1's digit
 *                                    ------------
 *
 *
 * Schematic of the bit numbers for the display LED's. Useful if custom characters are needed.
 *
 *           * 7       --------    *    --------       * C
 *                    /   7   /    1   /   7   /       5 2
 *                 2 /       / 6    2 /       / 6    ----
 *                   -------          -------     2 / 7 / 6
 *           *     /   1   /        /   1   /       ---
 *           3  5 /       / 3    5 /       / 3  5 / 1 / 3
 *                --------    *    --------   *   ----  *
 *                  4         0      4        0    4    0
 *
 *
 *
 *
 */

#ifndef __STC1000P_H__
#define __STC1000P_H__

#define __16f1828
#include "pic14/pic16f1828.h"

/* Define STC-1000+ version number (XYY, X=major, YY=minor) */
/* Also, keep track of last version that has changes in EEPROM layout */
#define STC1000P_VERSION			(109)
#define STC1000P_EEPROM_VERSION		(12)

/* Clear Watchdog */
#define ClrWdt() 					{ __asm CLRWDT __endasm; }

/* Special registers used as flags */ 
#define	MENU_IDLE				TMR1GE
#define	SENSOR_SELECT			TX9
#ifdef OVBSC
	#define RUN_PRG				C1POL
	#define ALARM				C2POL
	#define	PAUSE				C1HYS
	#define THERMOSTAT			C2HYS
	#define PUMP				C1SYNC
	#define UNUSED2				C2SYNC
	/* Initialized to 1 */
	#define OFF					C1SP

	// RA1 output, write high.
	#define	PUMP_ON()			do { TRISA1=0; LATA1=1; } while(0)
	// RA1 input, tristate
	#define PUMP_MANUAL()		do { TRISA1=1; LATA1=0; } while(0)
	// RA1 output, write low.
	#define PUMP_OFF()			do { TRISA1=0; LATA1=0; } while(0)
#else /* !OVBSC */
	#define	SHOW_SA_ALARM		RX9
#endif

/* Define limits for temperatures */
#ifdef FAHRENHEIT
	#define TEMP_MAX				(2500)
	#define TEMP_MIN				(-400)
	#define TEMP_CORR_MAX			(100)
	#define TEMP_CORR_MIN			(-100)
	#define TEMP_HYST_1_MAX			(100)
	#define TEMP_HYST_2_MAX			(500)
	#define SP_ALARM_MIN			(-800)
	#define SP_ALARM_MAX			(800)
#else  // CELSIUS
	#define TEMP_MAX				(1400)
	#define TEMP_MIN				(-400)
	#define TEMP_CORR_MAX			(50)
	#define TEMP_CORR_MIN			(-50)
	#define TEMP_HYST_1_MAX			(50)
	#define TEMP_HYST_2_MAX			(250)
	#define SP_ALARM_MIN			(-400)
	#define SP_ALARM_MAX			(400)
#endif

/* Defaults */
#ifdef FAHRENHEIT
	#if defined OVBSC
		#define DEFAULT_St			(153)
		#define DEFAULT_mt			(149)
	#else
		#define DEFAULT_hy			(10)
		#define DEFAULT_hy2			(100)
		#define DEFAULT_SP			(680)
	#endif
#else  // CELSIUS
	#if defined OVBSC
		#define DEFAULT_St			(670)
		#define DEFAULT_mt			(660)
	#else
		#define DEFAULT_hy			(5)
		#define DEFAULT_hy2			(50)
		#define DEFAULT_SP			(200)
	#endif
#endif

/* Enum to specify the types of the parameters in the menu. */
/* Note that this list needs to be ordered by how they should be presented on the display. */
/* The 'temperature types' should be first so that everything less or equal to t_sp_alarm */
/* (or t_tempdiff for OVBSC) will be presented as a temperature. */
/* t_runmode and t_period are handled as special cases, everything else is displayed as integers. */
enum e_item_type {
	t_temperature=0,
	t_tempdiff,
#ifdef OVBSC
	t_percentage,
	t_period,
	t_apflags,
	t_pumpflags,
#else
	t_hyst_1,
#if defined PB2
	t_hyst_2,
#endif
	t_sp_alarm,
#if defined FO433
	t_deviceid,
#endif
	t_step,
	t_delay,
	t_runmode,
#endif
	t_duration,
	t_boolean
};

#ifdef OVBSC
	#define MENU_TYPE_IS_TEMPERATURE(x) 	((x) <= t_tempdiff)
#else
	#define MENU_TYPE_IS_TEMPERATURE(x) 	((x) <= t_sp_alarm)
#endif

#if defined OVBSC
	#define MENU_DATA(_) \
		_(Sd, 	LED_S, 	LED_d, 	LED_OFF,	t_duration,			0)				\
		_(St, 	LED_S, 	LED_t, 	LED_OFF, 	t_temperature,		DEFAULT_St)		\
		_(SO, 	LED_S, 	LED_O, 	LED_OFF,	t_percentage,		150)			\
		_(Pt1, 	LED_P, 	LED_t, 	LED_1,		t_temperature,		DEFAULT_mt)		\
		_(Pd1, 	LED_P, 	LED_d, 	LED_1,		t_duration,			15)				\
		_(Pt2, 	LED_P, 	LED_t, 	LED_2,		t_temperature,		DEFAULT_mt)		\
		_(Pd2, 	LED_P, 	LED_d, 	LED_2,		t_duration,			30)				\
		_(Pt3, 	LED_P, 	LED_t, 	LED_3,	 	t_temperature,		DEFAULT_mt)		\
		_(Pd3, 	LED_P, 	LED_d, 	LED_3,		t_duration,			15)				\
		_(Pt4, 	LED_P, 	LED_t, 	LED_4,	 	t_temperature,		DEFAULT_mt)		\
		_(Pd4, 	LED_P, 	LED_d, 	LED_4,		t_duration,			0)				\
		_(Pt5, 	LED_P, 	LED_t, 	LED_5,	 	t_temperature,		DEFAULT_mt)		\
		_(Pd5, 	LED_P, 	LED_d, 	LED_5,		t_duration,			0)				\
		_(Pt6, 	LED_P, 	LED_t, 	LED_6,	 	t_temperature,		DEFAULT_mt)		\
		_(Pd6, 	LED_P, 	LED_d, 	LED_6,		t_duration,			0)				\
		_(PO, 	LED_P, 	LED_O, 	LED_OFF,	t_percentage,		50)				\
		_(Ht, 	LED_H, 	LED_t, 	LED_OFF,	t_temperature,		985)			\
		_(HO, 	LED_H, 	LED_O, 	LED_OFF,	t_percentage,		75)				\
		_(Hd, 	LED_H, 	LED_d, 	LED_OFF,	t_duration,			15)				\
		_(bO, 	LED_b, 	LED_O, 	LED_OFF,	t_percentage,		150)			\
		_(bd, 	LED_b, 	LED_d, 	LED_OFF,	t_duration,			90)				\
		_(hd1, 	LED_h, 	LED_d, 	LED_1,		t_duration,			60)				\
		_(hd2, 	LED_h, 	LED_d, 	LED_2,		t_duration,			45)				\
		_(hd3, 	LED_h, 	LED_d, 	LED_3, 		t_duration,			15)				\
		_(hd4, 	LED_h, 	LED_d, 	LED_4,		t_duration,			5)				\
		_(tc, 	LED_t, 	LED_c, 	LED_OFF,	t_tempdiff,			0)				\
		_(APF, 	LED_A, 	LED_P, 	LED_F,		t_apflags,			511)			\
		_(PF, 	LED_P, 	LED_F, 	LED_OFF,	t_pumpflags,		6)				\
		_(Pd, 	LED_P, 	LED_d, 	LED_OFF,	t_period,			50)				\
		_(cO, 	LED_c, 	LED_O, 	LED_OFF,	t_percentage,		80)				\
		_(cP, 	LED_c, 	LED_P, 	LED_OFF,	t_boolean,			0)				\
		_(cSP, 	LED_c, 	LED_S, 	LED_P, 		t_temperature,		0)				\
		_(ASd, 	LED_A, 	LED_S, 	LED_d, 		t_duration,			70)

#elif defined PB2
	/* The data needed for the 'Set' menu
	 * Using x macros to generate the data structures needed, all menu configuration can be kept in this
	 * single place.
	 *
	 * The values are:
	 * 	name, LED data 10, LED data 1, LED data 01, min value, max value, default value
	 */
	#define MENU_DATA(_) \
		_(SP, 	LED_S, 	LED_P, 	LED_OFF, 	t_temperature,		DEFAULT_SP)		\
		_(hy, 	LED_h, 	LED_y, 	LED_OFF, 	t_hyst_1,			DEFAULT_hy) 	\
		_(hy2, 	LED_h, 	LED_y, 	LED_2, 		t_hyst_2, 			DEFAULT_hy2)	\
		_(tc, 	LED_t, 	LED_c, 	LED_OFF, 	t_tempdiff,			0)				\
		_(tc2, 	LED_t, 	LED_c, 	LED_2, 		t_tempdiff,			0)				\
		_(SA, 	LED_S, 	LED_A, 	LED_OFF, 	t_sp_alarm,			0)				\
		_(St, 	LED_S, 	LED_t, 	LED_OFF, 	t_step,				0)				\
		_(dh, 	LED_d, 	LED_h, 	LED_OFF, 	t_duration,			0)				\
		_(cd, 	LED_c, 	LED_d, 	LED_OFF, 	t_delay,			5)				\
		_(hd, 	LED_h, 	LED_d, 	LED_OFF, 	t_delay,			2)				\
		_(rP, 	LED_r, 	LED_P, 	LED_OFF, 	t_boolean,			0)				\
		_(Pb, 	LED_P, 	LED_b, 	LED_2, 		t_boolean,			0)				\
		_(rn, 	LED_r, 	LED_n, 	LED_OFF, 	t_runmode,			6)

#elif defined FO433

	/* The data needed for the 'Set' menu
	 * Using x macros to generate the data structures needed, all menu configuration can be kept in this
	 * single place.
	 *
	 * The values are:
	 * 	name, LED data 10, LED data 1, LED data 01, min value, max value, default value
	 */
	#define MENU_DATA(_) \
		_(SP, 	LED_S, 	LED_P, 	LED_OFF, 	t_temperature,		DEFAULT_SP)		\
		_(hy, 	LED_h, 	LED_y, 	LED_OFF, 	t_hyst_1,			DEFAULT_hy) 	\
		_(tc, 	LED_t, 	LED_c, 	LED_OFF, 	t_tempdiff,			0)				\
		_(dI, 	LED_d, 	LED_I, 	LED_OFF, 	t_deviceid,			0)				\
		_(SA, 	LED_S, 	LED_A, 	LED_OFF, 	t_sp_alarm,			0)				\
		_(St, 	LED_S, 	LED_t, 	LED_OFF, 	t_step,				0)				\
		_(dh, 	LED_d, 	LED_h, 	LED_OFF, 	t_duration,			0)				\
		_(cd, 	LED_c, 	LED_d, 	LED_OFF, 	t_delay,			5)				\
		_(hd, 	LED_h, 	LED_d, 	LED_OFF, 	t_delay,			2)				\
		_(rP, 	LED_r, 	LED_P, 	LED_OFF, 	t_boolean,			0)				\
		_(rn, 	LED_r, 	LED_n, 	LED_OFF, 	t_runmode,			6)

#else

	/* The data needed for the 'Set' menu
	 * Using x macros to generate the data structures needed, all menu configuration can be kept in this
	 * single place.
	 *
	 * The values are:
	 * 	name, LED data 10, LED data 1, LED data 01, min value, max value, default value
	 */
	#define MENU_DATA(_) \
		_(SP, 	LED_S, 	LED_P, 	LED_OFF, 	t_temperature,		DEFAULT_SP)		\
		_(hy, 	LED_h, 	LED_y, 	LED_OFF, 	t_hyst_1,			DEFAULT_hy) 	\
		_(tc, 	LED_t, 	LED_c, 	LED_OFF, 	t_tempdiff,			0)				\
		_(SA, 	LED_S, 	LED_A, 	LED_OFF, 	t_sp_alarm,			0)				\
		_(St, 	LED_S, 	LED_t, 	LED_OFF, 	t_step,				0)				\
		_(dh, 	LED_d, 	LED_h, 	LED_OFF, 	t_duration,			0)				\
		_(cd, 	LED_c, 	LED_d, 	LED_OFF, 	t_delay,			5)				\
		_(hd, 	LED_h, 	LED_d, 	LED_OFF, 	t_delay,			2)				\
		_(rP, 	LED_r, 	LED_P, 	LED_OFF, 	t_boolean,			0)				\
		_(rn, 	LED_r, 	LED_n, 	LED_OFF, 	t_runmode,			6)

#endif

#define ENUM_VALUES(name, led10ch, led1ch, led01ch, type, default_value) \
    name,

/* Generate enum values for each entry int the set menu */
enum menu_enum {
    MENU_DATA(ENUM_VALUES)
};

/* Defines for EEPROM config addresses */
#ifdef OVBSC
	#define EEADR_MENU				0
#else
	#define NO_OF_PROFILES			6
	#define MENU_ITEM_NO			NO_OF_PROFILES
	#define THERMOSTAT_MODE			NO_OF_PROFILES

	#define EEADR_PROFILE_SETPOINT(profile, step)	(((profile)*19) + ((step)<<1))
	#define EEADR_PROFILE_DURATION(profile, step)	EEADR_PROFILE_SETPOINT(profile, step) + 1
	#define EEADR_MENU								EEADR_PROFILE_SETPOINT(NO_OF_PROFILES, 0)
	#define EEADR_POWER_ON							127
#endif
#define EEADR_MENU_ITEM(name)		(EEADR_MENU + (name))
#define MENU_SIZE					(sizeof(menu)/sizeof(menu[0]))

#define LED_OFF	0xff
#define LED_0	0x3
#define LED_1	0xb7
#define LED_2	0xd
#define LED_3	0x25
#define LED_4	0xb1
#define LED_5	0x61
#define LED_6	0x41
#define LED_7	0x37
#define LED_8	0x1
#define LED_9	0x21
#define LED_A	0x11
#define LED_a	0x5
#define LED_b	0xc1
#define LED_C	0x4b
#define LED_c	0xcd
#define LED_d	0x85
#define LED_e	0x9
#define LED_E	0x49
#define LED_F	0x59
#define LED_H	0x91
#define LED_h	0xd1
#define LED_I	0xb7
#define LED_J	0x87
#define LED_L	0xcb
#define LED_n	0xd5	
#define LED_O	0x3
#define LED_P	0x19
#define LED_r	0xdd	
#define LED_S	0x61
#define LED_t	0xc9
#define LED_U	0x83
#define LED_y	0xa1

/* Declare functions and variables from Page 0 */
#if defined COM
	#define COM_READ_EEPROM			0x20
	#define COM_WRITE_EEPROM		0xE0
	#define COM_READ_TEMP			0x01
	#define COM_READ_COOLING		0x02
	#define COM_READ_HEATING		0x03
	#define COM_ACK					0x9A
	#define COM_NACK				0x66
#endif

#if defined OVBSC
	static enum prg_state_enum {
		prg_off=0,
		prg_wait_strike,
		prg_strike,
		prg_strike_wait_alarm,
		prg_init_mash_step,
		prg_mash,
		prg_wait_boil_up_alarm,
		prg_init_boil_up,
		prg_hotbreak,
		prg_boil
	};
#endif

typedef union
{
	unsigned char raw;

	struct
	  {
	  unsigned                      : 1;
	  unsigned e_point              : 1;
	  unsigned e_c                  : 1;
	  unsigned e_heat               : 1;
	  unsigned e_negative           : 1;
	  unsigned e_deg                : 1;
	  unsigned e_set                : 1;
	  unsigned e_cool               : 1;
	  };
} led_e_t;

typedef union
{
	unsigned char raw;

	struct
	  {
	  unsigned decimal				: 1;
	  unsigned middle				: 1;
	  unsigned upper_left			: 1;
	  unsigned lower_right          : 1;
	  unsigned bottom				: 1;
	  unsigned lower_left			: 1;
	  unsigned upper_right			: 1;
	  unsigned top					: 1;
	  };
} led_t;

extern led_e_t led_e;
extern led_t led_10, led_1, led_01;
extern unsigned const char led_lookup[];

#if defined MINUTE
	extern int setpoint;
	extern unsigned int curr_dur;
#endif
#if defined OVBSC
	extern int setpoint;
	extern int output;
	extern unsigned char prg_state;
	extern unsigned int countdown;
	extern unsigned char mashstep;
#endif

extern unsigned int eeprom_read_config(unsigned char eeprom_address);
extern void eeprom_write_config(unsigned char eeprom_address,unsigned int data);
extern void value_to_led(int value, unsigned char decimal);
#define int_to_led(v)				value_to_led(v, 0)
#define temperature_to_led(v)		value_to_led(v, 1)
#define decimal_to_led(v)			value_to_led(v, 2);

/* Declare functions and variables from Page 1 */
extern void button_menu_fsm();

#endif // __STC1000P_H__
