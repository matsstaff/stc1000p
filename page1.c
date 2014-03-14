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

#define __16f1828
#include "pic14/pic16f1828.h"
#include "stc1000p.h"

/* Helpful defines to handle buttons */
#define BTN_PWR			0x88
#define BTN_S			0x44
#define BTN_UP			0x22
#define BTN_DOWN		0x11

#define BTN_IDLE(btn)		((_buttons & (btn)) == 0x00)
#define BTN_PRESSED(btn)	((_buttons & (btn)) == ((btn) & 0xf0))
#define BTN_HELD(btn)		((_buttons & (btn)) == (btn))
#define BTN_RELEASED(btn)	((_buttons & (btn)) == ((btn) & 0x0f))

/* Help to convert menu item number and config item number to an EEPROM config address */
#define ITEM_TO_ADDRESS(mi, ci)	((mi)*19 + (ci))

/* Button/menu FSM state */
unsigned char state=state_idle;

/* States for the menu FSM */
enum menu_states {
	state_idle = 0,
	state_power_down_wait,
	state_power_down_pre_off,
	state_power_down,

	state_show_menu_item,
	state_set_menu_item,
	state_show_config_item,
	state_set_config_item,
	state_show_config_value,
	state_set_config_value,

	state_up_pressed,
	state_down_pressed,
};

/* Helpers to constrain user input  */
#define RANGE(x,min,max)	(((x)>(max))?(min):((x)<(min))?(max):(x));

static int check_config_value(int config_value, unsigned char config_address){
	if(config_address < EEADR_PROFILE_SETPOINT(6,0)){
		if(config_address & 0x1){
			config_value = RANGE(config_value, 0, 999);
		} else {
			config_value = RANGE(config_value, TEMP_MIN, TEMP_MAX);
		}
	} else {
		switch(config_address){
		case EEADR_HYSTERESIS: // Hysteresis
			config_value = RANGE(config_value, 0, TEMP_CORR_MAX);
			break;
		case EEADR_TEMP_CORRECTION: // Temp correction
			config_value = RANGE(config_value, TEMP_CORR_MIN, TEMP_CORR_MAX);
			break;
		case EEADR_SETPOINT: // Setpoint
			config_value = RANGE(config_value, TEMP_MIN, TEMP_MAX);
			break;
		case EEADR_CURRENT_STEP: // Current step
			config_value = RANGE(config_value, 0, 8);
			break;
		case EEADR_CURRENT_STEP_DURATION: // Current duration
			config_value = RANGE(config_value, 0, 999);
			break;
		case EEADR_COOLING_DELAY: // Cooling delay
			config_value = RANGE(config_value, 0, 60);
			break;
		case EEADR_HEATING_DELAY: // Heating delay
			config_value = RANGE(config_value, 0, 4);
			break;
		case EEADR_RUN_MODE: // Run mode
			config_value = RANGE(config_value, 0, 6);
			break;
		}
	}
	return config_value;
}
/* This is the button input and menu handling function.
 * arguments: none
 * returns: nothing
 */
void button_menu_fsm(){
	static unsigned char menu_item=0, config_item=0, countdown=0;
	static int config_value;
	static unsigned char _buttons = 0;
	unsigned char _trisc, _portb;

	// Disable interrups while reading buttons
	GIE = 0;

	// Save registers that interferes with LED's
	_portb = PORTB;
	_trisc = TRISC;

	PORTB = 0b00000000; // Turn off LED's
	TRISC = 0b11011000; // Enable input for buttons

	_buttons = (_buttons << 1) | RC7; // pwr
	_buttons = (_buttons << 1) | RC4; // s
	_buttons = (_buttons << 1) | RC6; // up
	_buttons = (_buttons << 1) | RC3; // down

	// Restore registers
	PORTB = _portb;
	TRISC = _trisc;

	// Reenable interrups
	GIE = 1;

	if(countdown){
		countdown--;
	}

	switch(state){
	case state_idle:
		if(BTN_HELD(BTN_PWR)){
			countdown = 100; // 5 sec
			state = state_power_down_wait;
		} else if(BTN_RELEASED(BTN_S)){
			state = state_show_menu_item;
		}
		break;

	case state_power_down_wait:
		if(countdown==0 && BTN_RELEASED(BTN_PWR)){
			state = state_power_down;
		} else if(!BTN_HELD(BTN_PWR)){
			state = state_idle;
		}
		break;
	case state_power_down:
		// TODO Sleep until pwr btn pressed again
		state=state_idle;
		break;

	case state_show_menu_item:
		led_e |= (1<<4);
		if(menu_item < 6){
			led_10 = 0x19; // P
			led_1 = 0xdd; // r
			led_01 = led_lookup[menu_item];
		} else if(menu_item == 6) {
			led_10 = 0x61; // S
			led_1 = 0x9; // e
			led_01 = 0xc9;  // t
		}
		countdown = 200;
		state = state_set_menu_item;
		break;
	case state_set_menu_item:
		if(countdown==0 || BTN_RELEASED(BTN_PWR)){
			state=state_idle;
		} else if(BTN_RELEASED(BTN_UP)){
			menu_item = (menu_item == 6) ? 0 : menu_item+1;
			state = state_show_menu_item;
		} else if(BTN_RELEASED(BTN_DOWN)){
			menu_item = (menu_item == 0) ? 6 : menu_item-1;
			state = state_show_menu_item;
		} else if(BTN_RELEASED(BTN_S)){
			config_item = 0;
			state = state_show_config_item;
		}
		break;
	case state_show_config_item:
		led_e |= (1<<4);
		if(menu_item < 6){
			if(config_item & 0x1) {
				led_10 = 0x85; // d
				led_1 = 0xd1; // h
			} else {
				led_10 = 0x61; // S
				led_1 = 0x19; // P
			}
			led_01 = led_lookup[config_item >> 1];
		} else if(menu_item == 6){
			led_01 = 0xff;
			switch(config_item){
			case 0: // hysteresis
				led_10 = 0xd1; // h
				led_1 = 0xa1; // y
				break;
			case 1: // temp correction
				led_10 = 0xc9; // t
				led_1 = 0xcd; // c
				break;
			case 2: // Set point
				led_10 = 0x61; // S
				led_1 = 0x19; // P
				break;
			case 3: // profile step
				led_10 = 0x61; // S
				led_1 = 0xc9; // t
				break;
			case 4:  // profile duration
				led_10 = 0x85; // d
				led_1 = 0xd1; // h
				break;
			case 5: // Cooling delay
				led_10 = 0xcd; // c
				led_1 = 0x85; // d
				break;
			case 6: // Heating delay
				led_10 = 0xd1; // h
				led_1 = 0x85; // d
				break;
			case 7:
				led_10 = 0xdd; // r
				led_1 = 0xd5; // n
				break;
			}
		}
		countdown = 200;
		state = state_set_config_item;
		break;
	case state_set_config_item:
		if(countdown==0){
			state=state_idle;
		} else if(BTN_RELEASED(BTN_PWR)){
			state = state_show_menu_item;
		} else if(BTN_RELEASED(BTN_UP)){
			if(menu_item < 6){
				config_item = (config_item >= 18) ? 0 : config_item+1;
			} else {
				config_item = (config_item >= 7) ? 0 : config_item+1;
				if(config_item == 3 && (unsigned char)eeprom_read_config(EEADR_RUN_MODE) >= 6){
					config_item = 5;
				}
			}
			state = state_show_config_item;
		} else if(BTN_RELEASED(BTN_DOWN)){
			if(menu_item < 6){
				config_item = (config_item <= 0) ? 18 : config_item-1;
			} else {
				config_item = (config_item <= 0) ? 7 : config_item-1;
				if(config_item == 4 && (unsigned char)eeprom_read_config(EEADR_RUN_MODE) >= 6){
					config_item = 2;
				}
			}
			state = state_show_config_item;
		} else if(BTN_RELEASED(BTN_S)){
			config_value = eeprom_read_config(ITEM_TO_ADDRESS(menu_item, config_item));
			config_value = check_config_value(config_value, ITEM_TO_ADDRESS(menu_item, config_item));
			countdown = 200;
			state = state_show_config_value;
		}
		break;
	case state_show_config_value:
		if(menu_item < 6){
			if(config_item & 0x1){
				int_to_led(config_value);
			} else {
				temperature_to_led(config_value);
			}
		} else if(menu_item == 6){
			if(config_item < 3){
				temperature_to_led(config_value);
			} else if (config_item < 7){
				int_to_led(config_value);
			} else {
				if(config_value==6){
					led_10=0xc9; // t
					led_1=0xd1;	// h
					led_01 = 0xff;
				} else {
					led_10 = 0x19; // P
					led_1 = 0xdd; // r
					led_01 = led_lookup[config_value & 0xf];
				}
			}
		}
		countdown = 200;
		state = state_set_config_value;
		break;
	case state_set_config_value:
		if(countdown==0){
			state=state_idle;
		} else if(BTN_RELEASED(BTN_PWR)){
			state = state_show_config_item;
		} else if(BTN_RELEASED(BTN_UP) || BTN_HELD(BTN_UP)) {
			config_value = ((config_value >= 1000) || (config_value < -1000)) ? (config_value + 10) : (config_value + 1);
			config_value = check_config_value(config_value, ITEM_TO_ADDRESS(menu_item, config_item));
			state = state_show_config_value;
		} else if(BTN_RELEASED(BTN_DOWN) || BTN_HELD(BTN_DOWN)) {
			config_value = ((config_value > 1000) || (config_value <= -1000)) ? (config_value - 10) : (config_value - 1);
			config_value = check_config_value(config_value, ITEM_TO_ADDRESS(menu_item, config_item));
			state = state_show_config_value;
		} else if(BTN_RELEASED(BTN_S)){
			if(menu_item == 6){
				if(config_item == 7){
					// When setting runmode, clear current step & duration
					eeprom_write_config(EEADR_CURRENT_STEP, 0);
					eeprom_write_config(EEADR_CURRENT_STEP_DURATION, 0);
					if(config_value < 6){
						// Set intial value for SP
						eeprom_write_config(EEADR_SETPOINT, eeprom_read_config(EEADR_PROFILE_SETPOINT(config_value, 0)));
						// Hack in case inital step duration is '0'
						if(eeprom_read_config(EEADR_PROFILE_DURATION(config_value, 0)) == 0){
							config_value = 6;
						}
					}
				}
			}
			eeprom_write_config(ITEM_TO_ADDRESS(menu_item, config_item), config_value);
			state=state_show_config_item;
		}
		break;
	default:
		state=state_idle;
	}
}
