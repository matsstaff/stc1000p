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

/* Define STC-1000+ version number (XYY, X=major, YY=minor) */
/* Also, keep track of last version that has changes in EEPROM layout */
#define STC1000P_VERSION		106
#define STC1000P_EEPROM_VERSION	11

/* Define limits for temperatures */
#ifdef FAHRENHEIT
#define TEMP_MAX		(2500)
#define TEMP_MIN		(-400)
#define TEMP_CORR_MAX	(100)
#define TEMP_CORR_MIN	(-100)
#define TEMP_HYST_1_MAX	(100)
#define TEMP_HYST_2_MAX	(500)
#define SP_ALARM_MIN	(-800)
#define SP_ALARM_MAX	(800)
#else  // CELSIUS
#define TEMP_MAX		(1400)
#define TEMP_MIN		(-400)
#define TEMP_CORR_MAX	(50)
#define TEMP_CORR_MIN	(-50)
#define TEMP_HYST_1_MAX	(50)
#define TEMP_HYST_2_MAX	(250)
#define SP_ALARM_MIN	(-400)
#define SP_ALARM_MAX	(400)
#endif

/* The data needed for the 'Set' menu
 * Using x macros to generate the data structures needed, all menu configuration can be kept in this
 * single place.
 *
 * The values are:
 * 	name, LED data 10, LED data 1, LED data 01, min value, max value, default value celsius, default value fahrenheit
 */
#define SET_MENU_DATA(_) \
    _(hy, 	LED_h, 	LED_y, 	LED_OFF, 	0, 				TEMP_HYST_1_MAX,	5,		10) 	\
    _(hy2, 	LED_h, 	LED_y, 	LED_2, 		0, 				TEMP_HYST_2_MAX, 	50,		100)	\
    _(tc, 	LED_t, 	LED_c, 	LED_OFF, 	TEMP_CORR_MIN, 	TEMP_CORR_MAX,		0,		0)		\
    _(tc2, 	LED_t, 	LED_c, 	LED_2, 		TEMP_CORR_MIN,	TEMP_CORR_MAX,		0,		0)		\
    _(SA, 	LED_S, 	LED_A, 	LED_OFF, 	SP_ALARM_MIN,	SP_ALARM_MAX,		0,		0)		\
    _(SP, 	LED_S, 	LED_P, 	LED_OFF, 	TEMP_MIN,		TEMP_MAX,			200,	680)	\
    _(St, 	LED_S, 	LED_t, 	LED_OFF, 	0,				8,					0,		0)		\
    _(dh, 	LED_d, 	LED_h, 	LED_OFF, 	0,				999,				0,		0)		\
    _(cd, 	LED_c, 	LED_d, 	LED_OFF, 	0,				60,					5,		5)		\
    _(hd, 	LED_h, 	LED_d, 	LED_OFF, 	0,				60,					2,		2)		\
    _(rP, 	LED_r, 	LED_P, 	LED_OFF, 	0,				1,					0,		0)		\
    _(Pb, 	LED_P, 	LED_b, 	LED_2, 		0,				1,					0,		0)		\
    _(rn, 	LED_r, 	LED_n, 	LED_OFF, 	0,				6,					6,		6) 		\

#define ENUM_VALUES(name, led10ch, led1ch, led01ch, minv, maxv, dvc, dvf) \
    name,

/* Generate enum values for each entry int the set menu */
enum set_menu_enum {
    SET_MENU_DATA(ENUM_VALUES)
};

#define NO_OF_PROFILES							6
#define SET_MENU_ITEM_NO						NO_OF_PROFILES
#define THERMOSTAT_MODE							NO_OF_PROFILES

/* Defines for EEPROM config addresses */
#define EEADR_PROFILE_SETPOINT(profile, step)	(((profile)*19) + ((step)<<1))
#define EEADR_PROFILE_DURATION(profile, step)	EEADR_PROFILE_SETPOINT(profile, step) + 1
#define EEADR_SET_MENU							EEADR_PROFILE_SETPOINT(NO_OF_PROFILES, 0)
#define EEADR_SET_MENU_ITEM(name)				(EEADR_SET_MENU + (name))
#define EEADR_POWER_ON							127

#define SET_MENU_SIZE							(sizeof(setmenu)/sizeof(setmenu[0]))

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

extern unsigned int eeprom_read_config(unsigned char eeprom_address);
extern void eeprom_write_config(unsigned char eeprom_address,unsigned int data);
extern void value_to_led(int value, unsigned char decimal);
#define int_to_led(v)			value_to_led(v, 0);
#define temperature_to_led(v)	value_to_led(v, 1);

/* Declare functions and variables from Page 1 */
extern void button_menu_fsm();

#endif // __STC1000P_H__
