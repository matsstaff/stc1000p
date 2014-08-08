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
#else  // CELSIUS
#define TEMP_MAX		(1400)
#define TEMP_MIN		(-400)
#define TEMP_CORR_MAX	(50)
#define TEMP_CORR_MIN	(-50)
#define TEMP_HYST_1_MAX	(50)
#define TEMP_HYST_2_MAX	(250)
#endif

/* Defines for EEPROM config addresses */
#define EEADR_PROFILE_SETPOINT(profile, step)	(((profile)<<4) + ((profile)<<1) + (profile) + ((step)<<1))
#define EEADR_PROFILE_DURATION(profile, step)	EEADR_PROFILE_SETPOINT(profile, step) + 1
#define EEADR_HYSTERESIS						114
#define EEADR_HYSTERESIS_2						115
#define EEADR_TEMP_CORRECTION					116
#define EEADR_TEMP_CORRECTION_2					117
#define EEADR_SETPOINT							118
#define EEADR_CURRENT_STEP						119
#define EEADR_CURRENT_STEP_DURATION				120
#define EEADR_COOLING_DELAY						121
#define EEADR_HEATING_DELAY						122
#define EEADR_RAMPING							123
#define EEADR_2ND_PROBE							124
#define EEADR_RUN_MODE							125
#define EEADR_POWER_ON							127

/* Declare functions and variables from Page 0 */

typedef union
{
	unsigned char led_e;

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
} _led_e_bits;

extern _led_e_bits led_e;
extern unsigned char led_10, led_1, led_01;
extern unsigned const char led_lookup[];

extern unsigned int eeprom_read_config(unsigned char eeprom_address);
extern void eeprom_write_config(unsigned char eeprom_address,unsigned int data);
extern void value_to_led(int value, unsigned char decimal);
#define int_to_led(v)			value_to_led(v, 0);
#define temperature_to_led(v)	value_to_led(v, 1);

/* Declare functions and variables from Page 1 */
extern void button_menu_fsm();

#endif // __STC1000P_H__
