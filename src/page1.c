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

#define reset() { __asm RESET __endasm; }

typedef union {
	unsigned char raw;
	struct {
		unsigned BTN_DOWN_cur     : 1;
		unsigned BTN_UP_cur       : 1;
		unsigned BTN_S_cur        : 1;
		unsigned BTN_PWR_cur      : 1;
		unsigned BTN_DOWN_prev    : 1;
		unsigned BTN_UP_prev      : 1;
		unsigned BTN_S_prev       : 1;
		unsigned BTN_PWR_prev     : 1;
	};
} buttons_t;

/* Helpful defines to handle buttons */
#define BTN_IDLE(btn)               (!(_buttons.##btn##_cur || _buttons.btn##_prev))
#define BTN_PRESSED(btn)            (_buttons.btn##_cur && !_buttons.btn##_prev)
#define BTN_HELD(btn)               (_buttons.btn##_cur && _buttons.btn##_prev)
#define BTN_RELEASED(btn)           (!_buttons.btn##_cur && _buttons.btn##_prev)
#define BTN_HELD_OR_RELEASED(btn)   (_buttons.btn##_prev)
#define BTN_ANY_RELEASED()          (!(_buttons.raw & 0x0f) && (_buttons.raw & 0xf0))
#define BTN_ANY_ACTIVITY()          (_buttons.raw)

#ifndef OVBSC
	/* Help to convert menu item number and config item number to an EEPROM config address */
	#define MI_CI_TO_EEADR(mi, ci)		((mi)*19 + (ci))

	extern unsigned int heating_delay;
	extern unsigned int cooling_delay;
#endif

/* Menu struct */
struct s_menu {
    unsigned char led_c_10;
    unsigned char led_c_1;
    unsigned char led_c_01;
	unsigned char type;
};

/* Menu struct data generator */
#define TO_STRUCT(name, led10ch, led1ch, led01ch, type, default_value) \
    { led10ch, led1ch, led01ch, type },

static const struct s_menu menu[] = {
	MENU_DATA(TO_STRUCT)
};

/* Helpers to constrain user input  */
static int range(int x, int min, int max){
	if(x>max)
		return min;
	if(x<min)
		return max;
	return x;
}

/* Check and constrain a configuration value */
static int check_config_value(int config_value, unsigned char eeadr){
	int t_min = 0, t_max=999;
#if defined(OVBSC)
	if(eeadr == MENU_SIZE){
		t_max = 3;
	} else		
#elif defined(RH)
#else
	if(eeadr < EEADR_MENU){
		while(eeadr >= 19){
			eeadr-=19;
		}
		if(!(eeadr & 0x1)){
			t_min = TEMP_MIN;
			t_max = TEMP_MAX;
		}
	} else
#endif
	{
		unsigned char type = menu[eeadr - EEADR_MENU].type;
		if(type == t_temperature){
			t_min = TEMP_MIN;
			t_max = TEMP_MAX;
		} else if(type == t_tempdiff){
			t_min = TEMP_CORR_MIN;
			t_max = TEMP_CORR_MAX;
//		} else if(type == t_duration){
		} else if(type == t_boolean){
			t_max = 1;
#if defined(OVBSC)
		} else if(type == t_percentage){
			t_min = -200;
			t_max = 200;
		} else if(type == t_period){
			t_min = 10;
			t_max = 200;
		} else if(type == t_apflags){
			t_max = 511;
		} else if(type == t_pumpflags){
			t_max = 31;
#elif defined(RH)
		} else if(type == t_rh){
			t_max = 100;
		} else if(type == t_rhdiff){
			t_min = -10;
			t_max = 10;
		} else if(type == t_show_r_t){
			t_max = 3;
#else
		} else if(type == t_hyst_1){
			t_max = TEMP_HYST_1_MAX;
#if defined(FO433)
		} else if(type == t_deviceid){
			t_max = 15;
#endif
#if defined(FO433)
		} else if(type == t_deviceid){
			t_max = 8;
#endif
		} else if(type == t_sp_alarm){
			t_min = SP_ALARM_MIN;
			t_max = SP_ALARM_MAX;
		} else if(type == t_step){
			t_max = 8;
		} else if(type == t_delay){
			t_max = 60;
		} else if(type == t_runmode){
			t_max = 6;
#endif
		}
	}
	return range(config_value, t_min, t_max);
}

#if defined(OVBSC)
static void menu_to_led(unsigned char mi){
	led_e.e_negative = 1;
	led_e.e_deg = 1;
	led_e.e_c = 1;
	led_e.e_point = 1;
	if(mi < MENU_SIZE){
		led_10.raw = menu[mi].led_c_10;
		led_1.raw = menu[mi].led_c_1;
		led_01.raw = menu[mi].led_c_01;
	} else {
		led_10.raw = LED_r;
		led_1.raw = LED_U;
		led_01.raw = LED_n;
	}
}
#elif defined(RH)
static void menu_to_led(unsigned char mi){
	led_e.e_negative = 1;
	led_e.e_deg = 1;
	led_e.e_c = 1;
	led_e.e_point = 1;
	if(mi < MENU_SIZE){
		led_10.raw = menu[mi].led_c_10;
		led_1.raw = menu[mi].led_c_1;
		led_01.raw = menu[mi].led_c_01;
	}
}
#else // !OVBSC !RH

static void prx_to_led(unsigned char run_mode, unsigned char is_menu){
	led_e.e_negative = 1;
	led_e.e_deg = 1;
	led_e.e_c = 1;
	led_e.e_point = 1;
	if(run_mode<NO_OF_PROFILES){
		led_10.raw = LED_P;
		led_1.raw = LED_r;
		led_01.raw = led_lookup[run_mode];
	} else {
		if(is_menu){
			led_10.raw = LED_S;
			led_1.raw = LED_e;
			led_01.raw = LED_t;
		} else {
			led_10.raw = LED_t;
			led_1.raw = LED_h;
			led_01.raw = LED_OFF;
		}
	}
}

#define run_mode_to_led(x)	prx_to_led(x,0)
#define menu_to_led(x)		prx_to_led(x,1)

#endif

/* States for the menu FSM */
enum menu_states {
	menu_idle = 0,
	menu_show_version,
	menu_show_state_up,
	menu_show_state_down,
#if defined(RH)
	menu_reset_wait,
#else
	menu_show_state_down_2,
#endif
#if !(defined(OVBSC) || defined(RH))
	menu_show_state_down_3,
	menu_power_down_wait,
	menu_show_menu_item,
	menu_set_menu_item,
	menu_up_pressed,
	menu_down_pressed,
#endif
	menu_show_config_item,
	menu_set_config_item,
	menu_show_config_value,
	menu_set_config_value,
};

/* Due to a fault in SDCC, static local variables are not initialized
 * properly, so the variables below were moved from button_menu_fsm()
 * and made global.
 */
static unsigned char menustate=menu_idle;
#if (defined(OVBSC) || defined(RH))
static unsigned char config_item=0, m_countdown=0;
#else
static unsigned char menu_item=0, config_item=0, m_countdown=0;
#endif
static int config_value;
static buttons_t _buttons = { 0 };

/* This is the button input and menu handling function.
 * arguments: none
 * returns: nothing
 */
void button_menu_fsm(){
	{
		unsigned char trisc, latb;

		// Disable interrups while reading buttons
		GIE = 0;

		// Save registers that interferes with LED's
		latb = LATB;
		trisc = TRISC;

		LATB = 0b00000000; // Turn off LED's
		TRISC = 0b11011000; // Enable input for buttons

		_buttons.raw <<= 4;
		if (RC7) _buttons.BTN_PWR_cur = 1;
		if (RC4) _buttons.BTN_S_cur = 1;
		if (RC6) _buttons.BTN_UP_cur = 1;
		if (RC3) _buttons.BTN_DOWN_cur = 1;

		// Restore registers
		LATB = latb;
		TRISC = trisc;

		// Reenable interrups
		GIE = 1;
	}

	if(m_countdown){
		m_countdown--;
	}

	switch(menustate){
	case menu_idle:
#if defined(OVBSC)
		if(ALARM && (BTN_ANY_RELEASED())){
			ALARM = 0;
		} else if(BTN_RELEASED(BTN_PWR)){
			PAUSE = !PAUSE;
		} else {
#elif defined(RH)
		if(BTN_PRESSED(BTN_PWR)){
			m_countdown = 27; // 3 sec
			menustate = menu_reset_wait;
		} else {
#else
		if(BTN_PRESSED(BTN_PWR)){
			m_countdown = 27; // 3 sec
			menustate = menu_power_down_wait;
		} else if(BTN_ANY_ACTIVITY() && eeprom_read_config(EEADR_POWER_ON)){
#endif
			if (BTN_PRESSED(BTN_UP) || BTN_PRESSED(BTN_DOWN)){
				menustate = menu_show_version;
			} else if(BTN_PRESSED(BTN_UP)){
				menustate = menu_show_state_up;
			} else if(BTN_PRESSED(BTN_DOWN)){
				m_countdown = 13; // 1.5 sec
				menustate = menu_show_state_down;
			} else if(BTN_RELEASED(BTN_S)){
#if (defined(OVBSC) || defined(RH))
				menustate = menu_show_config_item;
#else
				menustate = menu_show_menu_item;
#endif
			}
		}
		break;

	case menu_show_version:
		int_to_led(STC1000P_VERSION);
		led_10.decimal = 0;
		led_e.e_deg = 1;
		led_e.e_c = 1;
		if (!(BTN_HELD(BTN_UP) || (BTN_HELD(BTN_DOWN)))){
			menustate=menu_idle;
		}
		break;

	case menu_show_state_up:
#if defined(OVBSC)
		if(OFF){
			led_10.raw = LED_O;
			led_1.raw = LED_F;
			led_01.raw = LED_F;
			led_e.raw = LED_OFF;
		} else if(PAUSE){
			led_10.raw = LED_P;
			led_1.raw = LED_S;
			led_01.raw = LED_E;
			led_e.raw = LED_OFF;
		} else if(THERMOSTAT){
			temperature_to_led(setpoint);
		} else {
			int_to_led(output);
		}
#elif defined(RH)
		int_to_led(eeprom_read_config(EEADR_COUNTER(eeprom_read_config(EEADR_COUNTER_INDEX))));
#else
#ifdef MINUTE
		temperature_to_led(setpoint);
#else
		temperature_to_led(eeprom_read_config(EEADR_MENU_ITEM(SP)));
#endif
#endif
		if(!BTN_HELD(BTN_UP)){
			menustate = menu_idle;
		}
		break;

	case menu_show_state_down:
#if defined(OVBSC)
		if(OFF){
			led_10.raw = LED_O;
			led_1.raw = LED_F;
			led_01.raw = LED_F;
			led_e.raw = LED_OFF;
		} else if(RUN_PRG){
			led_01.raw = LED_OFF;
			led_e.raw = LED_OFF;
			if(prg_state == prg_wait_strike){
				led_10.raw = LED_S;
				led_1.raw = LED_d;
			} else if(prg_state == prg_strike){
				led_10.raw = LED_S;
				led_1.raw = LED_t;
			} else if(prg_state == prg_init_mash_step){
				led_10.raw = LED_P;
				led_1.raw = LED_U;
				led_01.raw = led_lookup[mashstep+1];
			} else if(prg_state == prg_mash){
				led_10.raw = LED_OFF;
				led_1.raw = LED_P;
				led_01.raw = led_lookup[mashstep+1];
			} else if(prg_state == prg_init_boil_up){
				led_10.raw = LED_b;
				led_1.raw = LED_U;
			} else if(prg_state == prg_hotbreak){
				led_10.raw = LED_H;
				led_1.raw = LED_b;
			} else if(prg_state == prg_boil){
				led_10.raw = LED_b;
				led_1.raw = LED_OFF;
			}
		} else if(THERMOSTAT){
			led_10.raw = LED_C;
			led_1.raw = LED_t;
			led_01.raw = LED_OFF;
			led_e.raw = LED_OFF;
		} else{
			led_10.raw = LED_C;
			led_1.raw = LED_O;
			led_01.raw = LED_OFF;
			led_e.raw = LED_OFF;
		}
		if(m_countdown==0){
			m_countdown = 20;
			if(prg_state == prg_wait_strike || prg_state == prg_mash || prg_state >= prg_hotbreak){
				menustate = menu_show_state_down_2;
			}
		}
#elif defined(RH)
		// TODO Show something else?
		led_10.raw = LED_OFF;
		led_1.raw = LED_OFF;
		led_01.raw = LED_OFF;
		led_e.raw = LED_OFF;
#else
		{
			unsigned char run_mode = eeprom_read_config(EEADR_MENU_ITEM(rn));
			run_mode_to_led(run_mode);
			if(run_mode<THERMOSTAT_MODE && m_countdown==0){
				m_countdown=17;
				menustate = menu_show_state_down_2;
			}
		}
#endif
		if(!BTN_HELD(BTN_DOWN)){
			menustate = menu_idle;
		}
		break;

#if defined(RH)
	case menu_reset_wait:
		if(m_countdown==0){
			unsigned char cnti = eeprom_read_config(EEADR_COUNTER_INDEX);
			cnti = (cnti + 1) & 0xf;
			eeprom_write_config(EEADR_COUNTER(cnti), 0);
			eeprom_write_config(EEADR_COUNTER_INDEX, cnti);
			menustate = menu_idle;
		} else if(!BTN_HELD(BTN_PWR)){
			menustate = menu_idle;
		}
		break;
#endif

#if !defined(RH)
	case menu_show_state_down_2:
#if defined(OVBSC)
		int_to_led(countdown);
		if(m_countdown==0){
			m_countdown = 20;
			menustate = menu_show_state_down;
		}
#else
		int_to_led(eeprom_read_config(EEADR_MENU_ITEM(St)));
		if(m_countdown==0){
			m_countdown=13;
			menustate = menu_show_state_down_3;
		}
#endif
		if(!BTN_HELD(BTN_DOWN)){
			menustate = menu_idle;
		}
		break;
#endif // !RH

	case menu_show_config_item:
#if (defined(OVBSC) || defined(RH))
		menu_to_led(config_item);
#else
		led_e.e_negative = 1;
		led_e.e_deg = 1;
		led_e.e_c = 1;
		if(menu_item < MENU_ITEM_NO){
			if(config_item & 0x1) {
				led_10.raw = LED_d;
				led_1.raw = LED_h;
			} else {
				led_10.raw = LED_S;
				led_1.raw = LED_P;
			}
			led_01.raw = led_lookup[(config_item >> 1)];
		} else /* if(menu_item == 6) */{
			led_10.raw = menu[config_item].led_c_10;
			led_1.raw = menu[config_item].led_c_1;
			led_01.raw = menu[config_item].led_c_01;
		}
#endif
		m_countdown = 110;
		menustate = menu_set_config_item;
		break;

	case menu_set_config_item:
		if(m_countdown==0){
			menustate=menu_idle;
		} else if(BTN_RELEASED(BTN_PWR)){
#if defined(OVBSC)
			menustate=menu_idle;
		} else if(BTN_RELEASED(BTN_UP)){
			config_item++;
			if(config_item > MENU_SIZE){
				config_item = 0;
			}
			menustate = menu_show_config_item;
		} else if(BTN_RELEASED(BTN_DOWN)){
			config_item--;
			if(config_item > MENU_SIZE){
				config_item = MENU_SIZE;
			}
			menustate = menu_show_config_item;
		} else if(BTN_RELEASED(BTN_S)){
			if(config_item < MENU_SIZE){
				config_value = eeprom_read_config(config_item);
			} else {
				if(OFF){
					config_value=0;
				} else if(RUN_PRG){
					config_value=1;
				} else if(THERMOSTAT){
					config_value=2;
				} else {
					config_value = 3;
				}
			}
			menustate = menu_show_config_value;
		}
#elif defined(RH)
			menustate=menu_idle;
		} else if(BTN_RELEASED(BTN_UP)){
			config_item++;
			if(config_item >= MENU_SIZE){
				config_item = 0;
			}
			menustate = menu_show_config_item;
		} else if(BTN_RELEASED(BTN_DOWN)){
			config_item--;
			if(config_item >= MENU_SIZE){
				config_item = MENU_SIZE-1;
			}
			menustate = menu_show_config_item;
		} else if(BTN_RELEASED(BTN_S)){
			config_value = eeprom_read_config(config_item);
			menustate = menu_show_config_value;
		}
#else /* !OVBSC !RH */
			menustate = menu_show_menu_item;
		} else if(BTN_RELEASED(BTN_UP)){
			config_item++;
			if(menu_item < MENU_ITEM_NO){
				if(config_item >= 19){
					config_item = 0;
				}
			} else {
				if(config_item >= MENU_SIZE){
					config_item = 0;
				}
				/* Jump to exit code shared with BTN_DOWN case */
				/* GOTO's are frowned upon, but avoiding code duplication saves precious code space */
				goto chk_skip_menu_item;
			}
			menustate = menu_show_config_item;
		} else if(BTN_RELEASED(BTN_DOWN)){
			config_item--;
			if(menu_item < MENU_ITEM_NO){
				if(config_item > 18){
					config_item = 18;
				}
			} else {
				if(config_item > MENU_SIZE-1){
					config_item = MENU_SIZE-1;
				}
chk_skip_menu_item:
#if !defined(MINUTE)
				if((unsigned char)eeprom_read_config(EEADR_MENU_ITEM(rn)) >= THERMOSTAT_MODE)
#endif
				{
					if(config_item == St){
						config_item += 2;
					}else if(config_item == dh){
						config_item -= 2;
					}
				}
			}
			menustate = menu_show_config_item;
		} else if(BTN_RELEASED(BTN_S)){
			unsigned char adr = MI_CI_TO_EEADR(menu_item, config_item);
			config_value = eeprom_read_config(adr);
			config_value = check_config_value(config_value, adr);
			m_countdown = 110;
			menustate = menu_show_config_value;
		}
#endif 
		break;

		case menu_show_config_value:
#if defined(OVBSC)
			if(config_item < MENU_SIZE){
				unsigned char type = menu[config_item].type;
				if(MENU_TYPE_IS_TEMPERATURE(type)){
					temperature_to_led(config_value);
				} else if(type == t_period){
					decimal_to_led(config_value);
				} else {
					int_to_led(config_value);
				}
			} else {
				led_e.e_negative = 1;
				led_e.e_deg = 1;
				led_e.e_c = 1;
				led_e.e_point = 1;
				if(config_value==0){
					led_10.raw = LED_O;
					led_1.raw = LED_F;
					led_01.raw = LED_F;
				} else if(config_value==1){
					led_10.raw = LED_P;
					led_1.raw = LED_r;
					led_01.raw = LED_OFF;
				} else if(config_value==2){
					led_10.raw = LED_c;
					led_1.raw = LED_t;
					led_01.raw = LED_OFF;
				} else {
					led_10.raw = LED_c;
					led_1.raw = LED_O;
					led_01.raw = LED_OFF;
				}
			}
#elif defined(RH)
			{
				unsigned char type = menu[config_item].type;
				if(MENU_TYPE_IS_TEMPERATURE(type)){
					temperature_to_led(config_value);
				} else {
					int_to_led(config_value);
				}
			}
#else
		if(menu_item < MENU_ITEM_NO){
			if(config_item & 0x1){
				int_to_led(config_value);
			} else {
				temperature_to_led(config_value);
			}
		} else /* if(menu_item == MENU_ITEM_NO) */ {
			unsigned char type = menu[config_item].type;
			if(MENU_TYPE_IS_TEMPERATURE(type)){
				temperature_to_led(config_value);
			} else if (type == t_runmode){
				run_mode_to_led(config_value);
			} else {
				int_to_led(config_value);
			}
		}
#endif
			m_countdown = 110;
			menustate = menu_set_config_value;
			break;

	case menu_set_config_value:
		{
#if (defined(OVBSC) || defined(RH))
			unsigned char adr = config_item;
#else
			unsigned char adr = MI_CI_TO_EEADR(menu_item, config_item);
#endif
			if(m_countdown==0){
				menustate=menu_idle;
			} else if(BTN_RELEASED(BTN_PWR)){
				menustate = menu_show_config_item;
			} else if(BTN_HELD_OR_RELEASED(BTN_UP)) {
				config_value++;
				if(config_value > 1000){
					config_value+=9;
				}
				/* Jump to exit code shared with BTN_DOWN case */
				goto chk_cfg_acc_label;
			} else if(BTN_HELD_OR_RELEASED(BTN_DOWN)) {
				config_value--;
				if(config_value > 1000){
					config_value-=9;
				}
chk_cfg_acc_label:
				config_value = check_config_value(config_value, adr);
				if(PR6 > 30){
					PR6-=8;
				}
				menustate = menu_show_config_value;
			} else if(BTN_RELEASED(BTN_S)){
#if defined(OVBSC)
				if(config_item < MENU_SIZE){
					eeprom_write_config(config_item, config_value);
				} else {
					if(config_value==0){ // OFF
						OFF = 1;
						RUN_PRG=0;
						PUMP=0;
						THERMOSTAT = 0;
					} else if(config_value==1){ // Pr
						OFF=0;
						RUN_PRG=1;
					} else if(config_value==2){ // Ct
						OFF = 0;
						RUN_PRG=0;
						THERMOSTAT = 1;
					} else { // Co
						OFF = 0;
						RUN_PRG=0;
						THERMOSTAT = 0;
					}
				}
#elif defined(RH)
					eeprom_write_config(config_item, config_value);
#else
				if(menu_item == MENU_ITEM_NO){
					if(config_item == rn){
						// When setting runmode, clear current step & duration
						eeprom_write_config(EEADR_MENU_ITEM(St), 0);
#if defined(MINUTE)
						curr_dur = 0;
#else
						eeprom_write_config(EEADR_MENU_ITEM(dh), 0);
#endif
						if(config_value < THERMOSTAT_MODE){
							unsigned char eeadr_sp = EEADR_PROFILE_SETPOINT(((unsigned char)config_value), 0);
							// Set intial value for SP
#if defined(MINUTE)
							setpoint = eeprom_read_config(eeadr_sp);
							eeprom_write_config(EEADR_MENU_ITEM(SP), setpoint);
#else
							eeprom_write_config(EEADR_MENU_ITEM(SP), eeprom_read_config(eeadr_sp));
#endif
							// Hack in case inital step duration is '0'
							if(eeprom_read_config(eeadr_sp+1) == 0){
								config_value = THERMOSTAT_MODE;
							}
						}
					}
				}
				eeprom_write_config(adr, config_value);
#endif // !OVBSC !RH
				menustate=menu_show_config_item;
			} else {
				PR6 = 250;
			}
		}
		break;

#if !(defined(OVBSC) || defined(RH))
	case menu_power_down_wait:
		if(m_countdown==0){
			unsigned char pwr_on = eeprom_read_config(EEADR_POWER_ON);
			eeprom_write_config(EEADR_POWER_ON, !pwr_on);
			if(pwr_on){
				LATA0 = 0;
				LATA4 = 0;
				LATA5 = 0;
				TMR4ON = 0;
				TMR4IF = 0;
			} else {
				heating_delay=60;
				cooling_delay=60;
				TMR4ON = 1;
			}
			menustate = menu_idle;
		} else if(!BTN_HELD(BTN_PWR)){
#if defined(PB2)
			SENSOR_SELECT = !SENSOR_SELECT;
#endif
			menustate = menu_idle;
		}
		break;

	case menu_show_state_down_3: // profile duration
#if defined(MINUTE)
		int_to_led(curr_dur);
#else
		int_to_led(eeprom_read_config(EEADR_MENU_ITEM(dh)));
#endif
		if(m_countdown==0){
			m_countdown=13;
			menustate = menu_show_state_down;
		}
		if(!BTN_HELD(BTN_DOWN)){
			menustate=menu_idle;
		}
		break;

	case menu_show_menu_item:
		menu_to_led(menu_item);
		m_countdown = 110;
		menustate = menu_set_menu_item;
		break;
	case menu_set_menu_item:
		if(m_countdown==0 || BTN_RELEASED(BTN_PWR)){
			menustate=menu_idle;
		} else if(BTN_RELEASED(BTN_UP)){
			menu_item++;
			if(menu_item > MENU_ITEM_NO){
				menu_item = 0;
			}
			menustate = menu_show_menu_item;
		} else if(BTN_RELEASED(BTN_DOWN)){
			menu_item--;
			if(menu_item > MENU_ITEM_NO){
				menu_item = MENU_ITEM_NO;
			}
			menustate = menu_show_menu_item;
		} else if(BTN_RELEASED(BTN_S)){
			config_item = 0;
			menustate = menu_show_config_item;
		}
		break;
#endif // !(defined(OVBSC) || defined(RH))

	default:
		menustate = menu_idle;
		break;
	} /* switch(menustate) */

	MENU_IDLE = (menustate==menu_idle);
}
