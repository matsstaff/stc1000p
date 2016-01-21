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

#include "stc1000p.h"

/* Configuration words */
unsigned int __at _CONFIG1 __CONFIG1 = 0xFD4;
unsigned int __at _CONFIG2 __CONFIG2 = 0x3AFF;

/* Temperature lookup table  */
#ifdef FAHRENHEIT
	const int ad_lookup[] = { 0, -555, -319, -167, -49, 48, 134, 211, 282, 348, 412, 474, 534, 593, 652, 711, 770, 831, 893, 957, 1025, 1096, 1172, 1253, 1343, 1444, 1559, 1694, 1860, 2078, 2397, 2987 };
#else  // CELSIUS
	const int ad_lookup[] = { 0, -486, -355, -270, -205, -151, -104, -61, -21, 16, 51, 85, 119, 152, 184, 217, 250, 284, 318, 354, 391, 431, 473, 519, 569, 624, 688, 763, 856, 977, 1154, 1482 };
#endif

/* LED character lookup table (0-9) */
unsigned const char led_lookup[] = { LED_0, LED_1, LED_2, LED_3, LED_4, LED_5, LED_6, LED_7, LED_8, LED_9 };

/* Global variables to hold LED data (for multiplexing purposes) */
led_e_t led_e = {0xff};
led_t led_10, led_1, led_01;

/* Global state flags for various optimizations (uses fewer instructions and data space vs 1/0 chars) */
union {
	unsigned char raw;
	struct {
		unsigned ad_badrange       : 1;  // used for adc range checking
#if defined(PB2)
		unsigned probe2            : 1;  // cached flag indicating whether 2nd probe is active
#else
		unsigned                   : 1;
#endif
		unsigned                   : 1;
		unsigned                   : 1;
		unsigned                   : 1;
		unsigned                   : 1;
		unsigned                   : 1;
		unsigned                   : 1;
	};
} state_flags = {0};

static int temperature=0;

#if defined(OVBSC)
led_t al_led_10, al_led_1, al_led_01;
int setpoint=0;
int output=0;
static int thermostat_output=0;
#endif
#if defined(PB2)
static int temperature2=0;
#elif defined(COM)
static volatile unsigned char com_data=0;
static volatile unsigned char com_write=0;
static volatile unsigned char com_tmout=0;
static volatile unsigned char com_count=0;
static volatile unsigned char com_state=0;
#elif defined(FO433)
static volatile unsigned char fo433_data=0;
static unsigned char fo433_state=0;
static volatile unsigned char fo433_count=0;
static volatile unsigned char fo433_send_space=0;
static unsigned char fo433_sec_count = 24;
#endif
#if defined(MINUTE)
int setpoint;
unsigned int curr_dur = 0;
#endif
#if defined(RH)
#define AD_0_RH			169
#define AD_100_RH		813
static unsigned char 	humidity=0;
static unsigned int 	wait=0;
#endif

/* Functions.
 * Note: Functions used from other page cannot be static, but functions
 * not used from other page SHOULD be static to decrease overhead.
 * Functions SHOULD be defined before used (ie. not just declared), to
 * decrease overhead. Refer to SDCC manual for more info.
 */

/* Read one configuration data from specified address.
 * arguments: Config address (0-127)
 * return: the read data
 */
unsigned int eeprom_read_config(unsigned char eeprom_address){
	unsigned int data = 0;
	eeprom_address = (eeprom_address << 1);

	do {
		EEADRL = eeprom_address; // Data Memory Address to read
		CFGS = 0; // Deselect config space
		EEPGD = 0; // Point to DATA memory
		RD = 1; // Enable read

		data = ((((unsigned int) EEDATL) << 8) | (data >> 8));
	} while(!(eeprom_address++ & 0x1));

	return data; // Return data
}

/* Store one configuration data to the specified address.
 * arguments: Config address (0-127), data
 * return: nothing
 */
void eeprom_write_config(unsigned char eeprom_address,unsigned int data)
{
	// Avoid unnecessary EEPROM writes
	if(data == eeprom_read_config(eeprom_address)){
		return;
	}

	// multiply address by 2 to get eeprom address, as we will be storing 2 bytes.
	eeprom_address = (eeprom_address << 1);

	do {
		// Address to write
	    EEADRL = eeprom_address;
	    // Data to write
	    EEDATL = (unsigned char) data;
	    // Deselect configuration space
	    CFGS = 0;
	    //Point to DATA memory
	    EEPGD = 0;
	    // Enable write
	    WREN = 1;

	    // Disable interrupts during write
	    GIE = 0;

	    // Write magic words to EECON2
	    EECON2 = 0x55;
	    EECON2 = 0xAA;

	    // Initiate a write cycle
	    WR = 1;

	    // Re-enable interrupts
	    GIE = 1;

	    // Disable writes
	    WREN = 0;

	    // Wait for write to complete
	    while(WR);

	    // Clear write complete flag (not really needed
	    // as we use WR for check, but is nice)
	    EEIF=0;

	    // Shift data for next pass
	    data = data >> 8;

	} while(!(eeprom_address++ & 0x01)); // Run twice for 16 bits

}

static unsigned int divu10(unsigned int n) {
	unsigned int q, r;
	q = (n >> 1) + (n >> 2);
	q = q + (q >> 4);
	q = q + (q >> 8);
	q = q >> 3;
	r = n - ((q << 3) + (q << 1));
	return q + ((r + 6) >> 4);
}

/* Update LED globals with temperature or integer data.
 * arguments: value (actual temperature multiplied by 10 or an integer)
 *            decimal indicates if the value is multiplied by 10 (i.e. a temperature)
 * return: nothing
 */
void value_to_led(int value, unsigned char decimal) {
	unsigned char i;

	// Handle negative values
	if (value < 0) {
		led_e.e_negative = 0;
		value = -value;
	} else {
		led_e.e_negative = 1;
	}

	if(decimal==1){
		led_e.e_deg = 0;
#ifdef FAHRENHEIT
		led_e.e_c = 1;
#else
		led_e.e_c = 0;
#endif // FAHRENHEIT
	} else {
		led_e.e_deg = 1;
		led_e.e_c = 1;
	}

	// If temperature >= 100 we must lose decimal...
	if (value >= 1000) {
		value = divu10((unsigned int) value);
		decimal = 0;
	}

	// Convert value to BCD and set LED outputs
	if(value >= 100){
		for(i=0; value >= 100; i++){
			value -= 100;
		}
		led_10.raw = led_lookup[i & 0xf];
	} else {
		led_10.raw = LED_OFF; // Turn off led if zero (lose leading zeros)
	}
	if(value >= 10 || decimal || led_10.raw!=LED_OFF){ // If decimal, we want 1 leading zero
		for(i=0; value >= 10; i++){
			value -= 10;
		}
		led_1.raw = led_lookup[i];
		if(decimal){
			led_1.decimal = 0;
		}
	} else {
		led_1.raw = LED_OFF; // Turn off led if zero (lose leading zeros)
	}
	led_01.raw = led_lookup[(unsigned char)value];
}

#if defined OVBSC
static unsigned char output_counter=0;
static void output_control(){
	static unsigned char o, inv;
	
	if(output_counter == 0){
		output_counter = 100;
		if(output < 0){
			o = -output;
			inv = 1;
		}else{
			o = output;
			inv = 0;
		}
	}
	output_counter--;

	LATA0 = 0;
	led_e.e_point = 1;
	if(ALARM){
		LATA0 = (output_counter > 75);
	} else if(PAUSE && !OFF){
		led_e.e_point = output_counter & 0x1;
	}

	if(PAUSE || OFF){
		LATA5 = 0;
		LATA4 = 0;
		PUMP_OFF();
	} else {
		unsigned char e1=(o > output_counter);
		unsigned char e2=(o > (output_counter+100));

		if(inv){
			LATA5 = e2;
			LATA4 = e1;
		}else{
			LATA5 = e1;
			LATA4 = e2;
		}

		if(PUMP){
			PUMP_MANUAL();
		} else {
			PUMP_OFF();
		}

	}

	led_e.e_heat = !LATA5;
	led_e.e_cool = !LATA4;
}

static void temperature_control(){
	if(THERMOSTAT){
		if(temperature < setpoint){
			output = thermostat_output;
		} else {
			output = 0;
		}
	}
}

unsigned char prg_state=0;
unsigned int countdown=0;
unsigned char mashstep=0;
static void program_fsm(){
	static unsigned char sec_countdown=60;

	if(OFF){
		prg_state = prg_off;
		return;
	}

	if(PAUSE){
		return;
	}

	if(countdown){
		if(sec_countdown==0){
			sec_countdown=60;
			countdown--;
		}
		sec_countdown--;
	}

	switch(prg_state){
		case prg_off:
			if(RUN_PRG){
				countdown = eeprom_read_config(EEADR_MENU_ITEM(Sd)); 	/* Strike delay */
				prg_state = prg_wait_strike;
			} else {
				if(THERMOSTAT){
					setpoint = eeprom_read_config(EEADR_MENU_ITEM(cSP));
					thermostat_output = eeprom_read_config(EEADR_MENU_ITEM(cO));
				} else {
					output = eeprom_read_config(EEADR_MENU_ITEM(cO));
				}
				PUMP = eeprom_read_config(EEADR_MENU_ITEM(cP));
			}
		break;
		case prg_wait_strike:
			output = 0;
			THERMOSTAT = 0;
			PUMP=0;
			if(countdown == 0){
				countdown = eeprom_read_config(EEADR_MENU_ITEM(ASd));
				prg_state = prg_strike;
			}
		break;
		case prg_strike:
			setpoint = eeprom_read_config(EEADR_MENU_ITEM(St)); /* Strike temp */
			output = eeprom_read_config(EEADR_MENU_ITEM(SO));
			PUMP=(eeprom_read_config(EEADR_MENU_ITEM(PF)) >> 0) & 0x1;
			if(temperature >= setpoint){
				THERMOSTAT = 1;
				thermostat_output = eeprom_read_config(EEADR_MENU_ITEM(PO));
				ALARM = (eeprom_read_config(EEADR_MENU_ITEM(APF)) >> 0) & 0x1;
				al_led_10.raw = LED_S;
				al_led_1.raw = LED_t;
				al_led_01.raw = LED_OFF;
				countdown = eeprom_read_config(EEADR_MENU_ITEM(ASd));
				prg_state = prg_strike_wait_alarm;
			} else if(countdown == 0){
				OFF=1;
			}
		break;
		case prg_strike_wait_alarm:
			if(!ALARM){
				PAUSE = (eeprom_read_config(EEADR_MENU_ITEM(APF)) >> 1) & 0x1;
				mashstep=0;
				countdown = eeprom_read_config(EEADR_MENU_ITEM(ASd));
				prg_state = prg_init_mash_step;
			} else if(countdown == 0){
				OFF=1;
			}
		break;
		case prg_init_mash_step:
			setpoint = eeprom_read_config(EEADR_MENU_ITEM(Pt1) + (mashstep << 1)); /* Mash step temp */
			output = eeprom_read_config(EEADR_MENU_ITEM(SO));
			THERMOSTAT = 0;
			PUMP=(eeprom_read_config(EEADR_MENU_ITEM(PF)) >> 1) & 0x1;
			if(temperature >= setpoint || (eeprom_read_config(EEADR_MENU_ITEM(Pd1) + (mashstep << 1)) == 0)){
				countdown = eeprom_read_config(EEADR_MENU_ITEM(Pd1) + (mashstep << 1)); /* Mash step duration */
				prg_state = prg_mash;
			} else if(countdown==0){
				OFF=1;
			}
		break;
		case prg_mash:
			THERMOSTAT = 1;
			thermostat_output = eeprom_read_config(EEADR_MENU_ITEM(PO));
			PUMP=(eeprom_read_config(EEADR_MENU_ITEM(PF)) >> 2) & 0x1;
			if(countdown==0){
				mashstep++;
				if(mashstep < 6){
					countdown = eeprom_read_config(EEADR_MENU_ITEM(ASd));
					prg_state = prg_init_mash_step;
				} else {
					ALARM = (eeprom_read_config(EEADR_MENU_ITEM(APF)) >> 2) & 0x1;
					al_led_10.raw = LED_b;
					al_led_1.raw = LED_U;
					al_led_01.raw = LED_OFF;
					countdown = eeprom_read_config(EEADR_MENU_ITEM(ASd));
					prg_state = prg_wait_boil_up_alarm;
				}
			}
		break;
		case prg_wait_boil_up_alarm:
			if(!ALARM){
				PAUSE = (eeprom_read_config(EEADR_MENU_ITEM(APF)) >> 3) & 0x1;
				countdown = eeprom_read_config(EEADR_MENU_ITEM(ASd));
				prg_state = prg_init_boil_up;
			} else if(countdown == 0){
				OFF=1;
			}
		break;
		case prg_init_boil_up:
			THERMOSTAT = 0;
			PUMP=(eeprom_read_config(EEADR_MENU_ITEM(PF)) >> 3) & 0x1;
			output = eeprom_read_config(EEADR_MENU_ITEM(SO)); /* Boil output */
			if(temperature >= eeprom_read_config(EEADR_MENU_ITEM(Ht))){ /* Boil up temp */
				countdown = eeprom_read_config(EEADR_MENU_ITEM(Hd)); /* Hotbreak duration */
				prg_state = prg_hotbreak;
			} else if(countdown==0){
				OFF=1;
			}
		break;
		case prg_hotbreak:
			output = eeprom_read_config(EEADR_MENU_ITEM(HO)); /* Hot break output */
			PUMP=(eeprom_read_config(EEADR_MENU_ITEM(PF)) >> 4) & 0x1;
			if(countdown==0){
				countdown = eeprom_read_config(EEADR_MENU_ITEM(bd)); /* Boil duration */ 
				prg_state = prg_boil;
			}
		break;
		case prg_boil:
			PUMP = 0;
			output = eeprom_read_config(EEADR_MENU_ITEM(bO)); /* Boil output */
			{
				unsigned char i;
				for(i=0; i<4; i++){
					if(countdown == eeprom_read_config(EEADR_MENU_ITEM(hd1) + i) && sec_countdown > 57){ /* Hop timer */
						ALARM = (eeprom_read_config(EEADR_MENU_ITEM(APF)) >> (4+i)) & 0x1;
						al_led_10.raw = LED_h;
						al_led_1.raw = LED_d;
						al_led_01.raw = led_lookup[i+1];
						break;
					}
				}
			}
			if(countdown == 0){
				output = 0;
				THERMOSTAT = 0;
				OFF = 1;
				ALARM = (eeprom_read_config(EEADR_MENU_ITEM(APF)) >> 8) & 0x1;
				al_led_10.raw = LED_C;
				al_led_1.raw = LED_h;
				al_led_01.raw = LED_OFF;
				RUN_PRG = 0;
				prg_state = prg_off;
			}
		break;

	} // switch(prg_state)

}

#elif defined RH

#else // !OVBSC !RH

/* To be called once every hour on the hour.
 * Updates EEPROM configuration when running profile.
 */
static void update_profile(){
	unsigned char profile_no = eeprom_read_config(EEADR_MENU_ITEM(rn));

	// Running profile?
	if (profile_no < THERMOSTAT_MODE) {
		unsigned char curr_step = eeprom_read_config(EEADR_MENU_ITEM(St));
		unsigned char profile_step_eeaddr;
		unsigned int profile_step_dur;
		int profile_next_step_sp;
#if defined MINUTE
		curr_dur++;
#else
		unsigned int curr_dur = eeprom_read_config(EEADR_MENU_ITEM(dh)) + 1;
#endif

		// Sanity check
		if(curr_step > 8){
			curr_step = 8;
		}

		profile_step_eeaddr = EEADR_PROFILE_SETPOINT(profile_no, curr_step);
		profile_step_dur = eeprom_read_config(profile_step_eeaddr + 1);
		profile_next_step_sp = eeprom_read_config(profile_step_eeaddr + 2);

		// Reached end of step?
		if (curr_dur >= profile_step_dur) {
			// Update setpoint with value from next step
#if defined MINUTE
			setpoint = profile_next_step_sp;
#endif
			eeprom_write_config(EEADR_MENU_ITEM(SP), profile_next_step_sp);
			// Is this the last step (next step is number 9 or next step duration is 0)?
			if (curr_step == 8 || eeprom_read_config(profile_step_eeaddr + 3) == 0) {
				// Switch to thermostat mode.
				eeprom_write_config(EEADR_MENU_ITEM(rn), THERMOSTAT_MODE);
				return; // Fastest way out...
			}
			// Reset duration
			curr_dur = 0;
			// Update step
			curr_step++;
			eeprom_write_config(EEADR_MENU_ITEM(St), curr_step);
		} else if(eeprom_read_config(EEADR_MENU_ITEM(rP))) { // Is ramping enabled?
			int profile_step_sp = eeprom_read_config(profile_step_eeaddr);
			unsigned int t = curr_dur << 6;
			long sp = 32;
			unsigned char i;

			// Linear interpolation calculation of new setpoint (64 substeps)
			for (i = 0; i < 64; i++) {
			 if (t >= profile_step_dur) {
			    t -= profile_step_dur;
			    sp += profile_next_step_sp;
			  } else {
			    sp += profile_step_sp;
			  }
			}
			sp >>= 6;

			// Update setpoint
#if defined MINUTE
			setpoint = sp;
		}
#else
			eeprom_write_config(EEADR_MENU_ITEM(SP), sp);
		}
		// Update duration
		eeprom_write_config(EEADR_MENU_ITEM(dh), curr_dur);
#endif
	}
}

/* Due to a fault in SDCC, static local variables are not initialized
 * properly, so the variables below were moved from temperature_control()
 * and made global.
 */
unsigned int cooling_delay = 60;  // Initial cooling delay
unsigned int heating_delay = 60;  // Initial heating delay
static void temperature_control(){
#ifndef MINUTE
	int setpoint = eeprom_read_config(EEADR_MENU_ITEM(SP));
#endif
#if defined PB2
	int hysteresis2 = eeprom_read_config(EEADR_MENU_ITEM(hy2));
#endif

	if(cooling_delay){
		cooling_delay--;
	}
	if(heating_delay){
		heating_delay--;
	}

	// Set LED outputs
	led_e.e_cool = !LATA4;
	led_e.e_heat = !LATA5;

	// This is the thermostat logic
#if defined PB2
	if((LATA4 && (temperature <= setpoint || (state_flags.probe2 && (temperature2 < (setpoint - hysteresis2))))) || (LATA5 && (temperature >= setpoint || (state_flags.probe2 && (temperature2 > (setpoint + hysteresis2)))))){
#else
	if((LATA4 && (temperature <= setpoint )) || (LATA5 && (temperature >= setpoint))){
#endif
		cooling_delay = eeprom_read_config(EEADR_MENU_ITEM(cd)) << 6;
		cooling_delay = cooling_delay - (cooling_delay >> 4);
		heating_delay = eeprom_read_config(EEADR_MENU_ITEM(hd)) << 6;
		heating_delay = heating_delay - (heating_delay >> 4);
		LATA4 = 0;
		LATA5 = 0;
	}
	else if(LATA4 == 0 && LATA5 == 0) {
		int hysteresis = eeprom_read_config(EEADR_MENU_ITEM(hy));
#ifdef PB2
		hysteresis2 >>= 2; // Halve hysteresis 2
		if ((temperature > setpoint + hysteresis) && (!state_flags.probe2 || (temperature2 >= setpoint - hysteresis2))) {
#else
		if (temperature > setpoint + hysteresis) {
#endif
			if (cooling_delay) {
				led_e.e_cool = led_e.e_cool ^ (cooling_delay & 0x1); // Flash to indicate cooling delay
			} else {
				LATA4 = 1;
			}
#if defined PB2
		} else if ((temperature < setpoint - hysteresis) && (!state_flags.probe2 || (temperature2 <= setpoint + hysteresis2))) {
#else
		} else if (temperature < setpoint - hysteresis) {
#endif
			if (heating_delay) {
				led_e.e_heat = led_e.e_heat ^ (heating_delay & 0x1); // Flash to indicate heating delay
			} else {
				LATA5 = 1;
			}
		}
	}
}

#endif // !OVBSC

/* Initialize hardware etc, on startup.
 * arguments: none
 * returns: nothing
 */
static void init() {

	OSCCON = 0b01101010; // 4MHz

	// Heat, cool as output, Thermistor as input, piezo output
#if (defined(FO433) || defined(OVBSC))
	TRISA = 0b00001100;
#else
	TRISA = 0b00001110;
#endif
	LATA = 0; // Drive relays and piezo low

#if defined(OVBSC)
	// Make sure weak pullup is disabled for RA1
	WPUA1 = 0;
#endif

	// LED Common anodes
	TRISB = 0;
	LATB = 0;

	// LED data (and buttons) output
	TRISC = 0;

	// Analog input on thermistor
#if (defined(PB2) || defined(RH))
	ANSELA = _ANSA1 |  _ANSA2 ;
#else
	ANSELA =  _ANSA2;
#endif

	// AD clock FOSC/8 (FOSC = 4MHz)
	ADCS0 = 1;
	// Right justify AD result
	ADFM = 1;

	// IMPORTANT FOR BUTTONS TO WORK!!! Disable analog input -> enables digital input
	ANSELC = 0;

	// Postscaler 1:1, Enable counter, prescaler 1:4
	T2CON = 0b00000101;
	// @4MHz, Timer 2 clock is FOSC/4 -> 1MHz prescale 1:4-> 250kHz, 250 gives interrupt every 1 ms
	PR2 = 250;
	// Enable Timer2 interrupt
	TMR2IE = 1;

	// Postscaler 1:15, - , prescaler 1:16
	T4CON = 0b01110010;
#if (defined(OVBSC) || defined(RH))
	TMR4ON = 1;
#else
	TMR4ON = eeprom_read_config(EEADR_POWER_ON);
#endif
	// @4MHz, Timer 2 clock is FOSC/4 -> 1MHz prescale 1:16-> 62.5kHz, 250 and postscale 1:15 -> 16.66666 Hz or 60ms
	PR4 = 250;

	// Postscaler 1:7, Enable counter, prescaler 1:64
	T6CON = 0b00110111;
	// @4MHz, Timer 2 clock is FOSC/4 -> 1MHz prescale 1:64-> 15.625kHz, 250 and postscale 1:6 -> 8.93Hz or 112ms
	PR6 = 250;

#if defined(MINUTE)
	// Get initial setpoint
	setpoint = eeprom_read_config(EEADR_MENU_ITEM(SP));
#endif

#if defined(RH)
	HEATING = 0;
	HUMID = 0;
	wait = eeprom_read_config(EEADR_MENU_ITEM(don));
#endif

#if defined(COM)
	// Set PEIE and GIE (enable global interrupts), IOCIE
	INTCON = 0b11001000;
	IOCAP = 2;	// Rising edge detect
	IOCAN = 0;
#else
	// Set PEIE (enable peripheral interrupts, that is for timer2) and GIE (enable global interrupts)
	INTCON = 0b11000000;
#endif

#if defined(FO433)
	// Timer 0 prescale 8
	PSA=0;
	PS0=0;
	PS1=1;
	PS2=0;
#endif

}

/* Interrupt service routine.
 * Receives timer2 interrupts every millisecond.
 * Handles multiplexing of the LEDs.
 */
#if defined(OVBSC)
volatile unsigned char oc = 0;
#endif
static void interrupt_service_routine(void) __interrupt 0 {

#if defined(COM)
	if(IOCAF1){

		/* Disable pin change interrupt and edge detection */
		IOCIE = 0;
		IOCAP1 = 0;

		/* If sending a '1' bit */
		if(com_write && (com_data & 0x80)){
			TRISA1 = 0;
			LATA1 = 1;
		}

		/* Init communication reset countdown (10ms) */
		com_tmout = 10;

		/* Enable timer 0 to generate interrupt for reading/writing in 250us */
		TMR0 = 5;
		TMR0CS = 0;
		TMR0IE = 1;

		/* Clear edge detection flag */
		IOCAF = 0;	
	}

	if(TMR0IF){
		/* Disable timer 0 */
		TMR0CS = 1;
		TMR0IE = 0;

		/* Make sure RA1 is input */
		TRISA1 = 1;
		LATA1 = 0;

		/* Shift data */
		com_data <<= 1;
		com_count++;

		/* If receive '1' */
		if(!com_write && RA1){
			com_data |= 1;
		}

		/* Enable edge detection and pin change interrupt */
		IOCAF = 0;
		IOCAP1 = 1;
		IOCIE = 1;

		/* Clear timer 0 interrupt flag */
		TMR0IF = 0;
	}
#elif defined(FO433)
	if(TMR0IF){
		/* */
		LATA1 = 0;

		if(fo433_send_space){
			fo433_count>>=1;
			TMR0 = 130;
			fo433_send_space = 0;
		} else {
			if(fo433_data & fo433_count){
				TMR0 = 193;
			} else {
				TMR0 = 67;
			}
			LATA1 = 1;
			fo433_send_space = 1;
		}

		if(fo433_count == 0){
			/* Disable timer 0 */
			TMR0CS = 1;
			TMR0IE = 0;
		}

		/* Clear timer 0 interrupt flag */
		TMR0IF = 0;
	}
#endif
	// Check for Timer 2 interrupt
	// Kind of excessive when it's the only enabled interrupt
	// but is nice as reference if more interrupts should be needed
	if (TMR2IF) {
		unsigned char latb = (LATB << 1);

		if(latb == 0){
			latb = 0x10;
		}

		TRISC = 0; // Ensure LED data pins are outputs
		LATB = 0; // Disable LED's while switching

		// Multiplex LED's every millisecond
		switch(latb) {
			case 0x10:
			LATC = led_10.raw;
			break;
			case 0x20:
			LATC = led_1.raw;
			break;
			case 0x40:
			LATC = led_01.raw;
			break;
			case 0x80:
			LATC = led_e.raw;
			break;
		}

		// Enable new LED
		LATB = latb;

#if defined(OVBSC)
		if(oc){
			oc--;
		}
#endif

#if defined(COM)
		/* Reset communication state on timeout */
		if(com_tmout){
			com_tmout--;
		} else {
			com_state = 0;
			com_count = 0;
			com_write = 0;
		}
#endif // COM

		// Clear interrupt flag
		TMR2IF = 0;
	}
}

#define START_TCONV_1()		(ADCON0 = _CHS1 | _ADON)
#define START_TCONV_2()		(ADCON0 = _CHS0 | _ADON)
#define FILTER_SHIFT		6
static unsigned int read_ad(unsigned int adfilter){
	unsigned char adfilter_l = adfilter >> 8;
	ADGO = 1;
	while(ADGO);
	ADON = 0;
	if ((adfilter_l >= 248) || (adfilter_l <=8 )) {
		state_flags.ad_badrange = 1;
	}
	return ((adfilter - (adfilter >> FILTER_SHIFT)) + ((ADRESH << 8) | ADRESL));
}

static int ad_to_temp(unsigned int adfilter){
	unsigned char i;
	long temp = 32;
	unsigned char a = ((adfilter >> (FILTER_SHIFT-1)) & 0x3f); // Lower 6 bits
	unsigned char b = ((adfilter >> (FILTER_SHIFT+5)) & 0x1f); // Upper 5 bits

	// Interpolate between lookup table points
	for (i = 0; i < 64; i++) {
		if(a <= i) {
			temp += ad_lookup[b];
		} else {
			temp += ad_lookup[b + 1];
		}
	}

	// Divide by 64 to get back to normal temperature
	return (temp >> 6);
}

#if defined(RH)
static unsigned char ad_to_rh(unsigned int adfilter){
	int rh = (adfilter >> FILTER_SHIFT) - AD_0_RH;	
	rh = (rh >> 3) + (rh >> 5);
	rh += eeprom_read_config(EEADR_MENU_ITEM(rhc));
	if(rh < 0){
		rh = 0;
	} else if(rh > 100){
		rh = 100;
	}
	return rh;
}

static unsigned char limithist=0;
static unsigned char hcount = 0;
static void control_rh(){

	unsigned humlimit = 100;
	
	if(temperature > 0 && temperature < 500){
		unsigned char i=0, j;
		int t = temperature;
		int rh=0;
		while(t >= 50) {
			t -= 50;
			i++;
		}
		t <<= 2;
		for(j=0; j<8; j++){
			if(t>=25){
				t-=25;
				rh+=eeprom_read_config(i+1);
			} else {
				rh+=eeprom_read_config(i);
			}
		}
		humlimit = (rh >> 3);
	}

	limithist = (limithist << 1) | (humidity > humlimit);

	{
		unsigned char ones=0;
		unsigned char c = limithist;
		while(c){
			if(c & 1){
				ones++;
			}
			c = (c >> 1);
		}

		if(HEATING){
			HUMID = (ones > 2);
		} else {
			HUMID = (ones > 5);
		}
	}

	if(hcount==0){
		if(wait!=0){
			wait--;
		}

		if(HEATING){
			unsigned char cntadr = EEADR_COUNTER(eeprom_read_config(EEADR_COUNTER_INDEX));
			if(!HUMID){
				if(wait==0){
					HEATING = 0;
					wait = eeprom_read_config(EEADR_MENU_ITEM(don));
				}
			}
			eeprom_write_config(cntadr, eeprom_read_config(cntadr) + 1);
		} else {
			if(!HUMID) {
				wait = eeprom_read_config(EEADR_MENU_ITEM(don));
			} else if(wait==0){
				HEATING = 1;
				wait = eeprom_read_config(EEADR_MENU_ITEM(dff));
			}
		}
	}
	hcount=((hcount+1) & 0x7);

	LATA4 = HEATING;
	LATA5 = HEATING;

	led_e.e_cool = !HEATING;
	led_e.e_heat = !HEATING;
}

#endif // RH

#if defined(COM)
enum com_states {
	com_idle = 0,
	com_recv_addr,
	com_recv_data1,
	com_recv_data2,
	com_recv_checksum,
	com_trans_data1,
	com_trans_data2,
	com_trans_checksum,
	com_trans_ack
};

/* State machine to handle rx/tx protocol */
static void handle_com(unsigned const char rxdata){
	static unsigned char command;
	static unsigned char xorsum;
	static unsigned int data;
	static unsigned char addr;

	xorsum ^= rxdata;

	com_write = 0;

	if(com_state == com_idle){
		command = rxdata;
		xorsum = rxdata;
		addr = 0;
		if(command == COM_READ_EEPROM || command == COM_WRITE_EEPROM){
			com_state = com_recv_addr;
		} else if(rxdata == COM_READ_TEMP){
			data = temperature;
			com_state = com_trans_data1;
		} else if(rxdata == COM_READ_COOLING){
			data = LATA4;
			com_state = com_trans_data1;
		} else if(rxdata == COM_READ_HEATING){
			data = LATA5;
			com_state = com_trans_data1;
		}
	} else if(com_state == com_recv_addr){
		addr = rxdata;
		if(command == COM_WRITE_EEPROM){
			com_state = com_recv_data1;
		} else {
			data = (unsigned int)eeprom_read_config(addr);
			com_state = com_trans_data1;
		}			
	} else if(com_state == com_recv_data1 || com_state == com_recv_data2){
		data = (data << 8) | rxdata;
		com_state++;
	} else if(com_state == com_recv_checksum){
		if(xorsum == 0){
			eeprom_write_config(addr, (int)data);
			com_data = COM_ACK;
		} else {
			com_data = COM_NACK;
		}
		com_write = 1;
		com_state = com_idle;
	} else if(com_state == com_trans_data2){
		com_data = (unsigned char) data; 
		com_write = 1;
		com_state = com_trans_checksum;
	} else if(com_state == com_trans_checksum){
		com_data = command ^ addr ^ ((unsigned char)(data >> 8)) ^ ((unsigned char) data);
		com_write = 1;
		com_state = com_trans_ack;
	} else if(com_state == com_trans_ack){
		com_data = COM_ACK;
		com_write = 1;
		com_state = com_idle;
	}

	if(com_state == com_trans_data1 || com_state == com_trans_data2){
		com_data = (data >> 8);
		com_write = 1;
		com_state++;
	}

}

#elif defined(FO433)

enum fo433_states {
	fo433_preamble=0,
	fo433_devid_high,
	fo433_devid_low_temp_high,
	fo433_temp_low,
	fo433_humidity,
	fo433_crc
};

/* Fine offset state machine */
static void fo433_fsm(){
	static int t;
	static unsigned char crc;

	if(fo433_state == fo433_preamble){
		fo433_data = 0xff;
	} else if(fo433_state == fo433_devid_high){
		fo433_data = 0x45;
		crc = 0;
	} else if(fo433_state == fo433_devid_low_temp_high){
		unsigned char di = ((unsigned char)eeprom_read_config(EEADR_MENU_ITEM(dI))) << 4;
		t = temperature;
		fo433_data = (di | ((t >> 8) & 0xf));
	} else if(fo433_state == fo433_temp_low){
		fo433_data = (unsigned char) t;
	} else if(fo433_state == fo433_humidity){
		fo433_data = (LATA5 << 6) | (LATA4 << 4);
	} else if(fo433_state == fo433_crc){
		fo433_data = crc;
	}

	// Calc CRC
	{
		unsigned char i=8;
		unsigned char d = fo433_data;
		while(i-- > 0){
			unsigned char mix = (crc ^ d) & 0x80;
			crc <<= 1;
			if(mix){
				crc ^= 0x31;
			}
			d <<= 1;
		}
	}

	fo433_count = 0x80;

	/* Enable timer 0 */
	TMR0IE = 1;
	TMR0CS = 0;
}
#endif // FO433

/*
 * Main entry point.
 */
void main(void) __naked {
	unsigned int millisx60 = 0;
	unsigned int ad_filter = (512L << FILTER_SHIFT);
#if defined(PB2) || defined(RH)
	unsigned int ad_filter2 = (512L << FILTER_SHIFT);
#endif

	init();

	START_TCONV_1();

	//Loop forever
	while (1) {

#if defined(COM)
		/* Handle communication if full byte transmitted/received */
		GIE = 0;
		if(com_count >= 8){
			char rxdata = com_data;
			com_count = 0;
			GIE = 1;
 			handle_com(rxdata);
		}
		GIE = 1;
#elif defined(FO433)
		/* Send next byte */
		if((fo433_state <= fo433_crc) && (fo433_count == 0)){
			fo433_fsm();
			fo433_state++;
		}
#endif

		if(TMR6IF) {

			// Handle button press and menu
			button_menu_fsm();

			if(!TMR4ON){
				led_e.raw = LED_OFF;
				led_10.raw = LED_O;
				led_1.raw = led_01.raw = LED_F;
			}

			// Reset timer flag
			TMR6IF = 0;
		}

#if defined(OVBSC)
		if(oc==0){
			oc = eeprom_read_config(EEADR_MENU_ITEM(Pd));
			output_control();
		}
#endif

		if(TMR4IF) {

			millisx60++;

			if(millisx60 & 0x1){
				ad_filter = read_ad(ad_filter);
#if (defined(PB2) || defined(RH))
				START_TCONV_2();
			} else {
				ad_filter2 = read_ad(ad_filter2);
#endif
				START_TCONV_1();
			}

			// Only run every 16th time called, that is 16x60ms = 960ms
			// Close enough to 1s for our purposes.
			if((millisx60 & 0xf) == 0) {

				temperature = ad_to_temp(ad_filter) + eeprom_read_config(EEADR_MENU_ITEM(tc));
#if defined(PB2)
				temperature2 = ad_to_temp(ad_filter2) + eeprom_read_config(EEADR_MENU_ITEM(tc2));
#endif
#if defined(RH)
				humidity = ad_to_rh(ad_filter2);
#endif

#if defined(FO433)
				if(fo433_sec_count < 3){
					fo433_state = 0;
				}
				if(fo433_sec_count == 0){
					fo433_sec_count = 48;
				}
				fo433_sec_count--;
#endif

#if defined(OVBSC)
				program_fsm();
				temperature_control();

				if(MENU_IDLE){
					if(ALARM && (millisx60 & 0x10)){
						led_10.raw = al_led_10.raw;
						led_1.raw = al_led_1.raw;
						led_01.raw = al_led_01.raw;
					} else {
						if(RUN_PRG && (prg_state == prg_boil || prg_state == prg_wait_strike)){
							int_to_led(countdown);
						} else {
							temperature_to_led(temperature);
						}
					}
				}

				if(millisx60 >= 1000){
					millisx60 = 8;
				}

#elif defined(RH)
				// Control RH every 7.5 min
				if(millisx60 >= 7500){
					control_rh();
					millisx60 = 0;
				}

				if(MENU_IDLE){
					unsigned char show_r_t = eeprom_read_config(EEADR_MENU_ITEM(Srt));
					led_e.e_set = !((HEATING ^ HUMID) && (millisx60 & 0x10));
					if((show_r_t == 0x2) || ((show_r_t & 0x2) && (millisx60 & 0x20))){
						int_to_led(humidity);
					} else if(show_r_t & 0x1){
						temperature_to_led(temperature);
					} else {
						led_01.raw = led_1.raw = led_10.raw = LED_OFF;
					}
				}
#else
				// Alarm on sensor error (AD result out of range)
				if (state_flags.ad_badrange) {
					LATA0 = 1;
				} else {
					LATA0 = 0;
				}
				// unlatch badrange flag for next iteration
				state_flags.ad_badrange = 0;
#if defined(PB2)
				// cache whether the 2nd probe is enabled or not.
				state_flags.probe2 = 0;
				if (eeprom_read_config(EEADR_MENU_ITEM(Pb))) {
					state_flags.probe2 = 1;
				}
#endif

				if(LATA0){ // On alarm, disable outputs
					led_10.raw = LED_A;
					led_1.raw = LED_L;
					led_e.raw = led_01.raw = LED_OFF;
					LATA4 = 0;
					LATA5 = 0;
					cooling_delay = heating_delay = 60;
				} else {
					// Update running profile every hour (if there is one)
					// and handle reset of millis x60 counter
					if(((unsigned char)eeprom_read_config(EEADR_MENU_ITEM(rn))) < THERMOSTAT_MODE){
						// Indicate profile mode
						led_e.e_set = 0;
#if defined(MINUTE)
						// Update profile every minute
						if(millisx60 >= 1000){
							update_profile();
							millisx60 = 8;
						}
#else
						// Update profile every hour
						if(millisx60 >= 60000){
							update_profile();
							millisx60 = 0;
						}
#endif
					} else {
						led_e.e_set = 1;
						millisx60 = 0;
					}

					{
						int sa = eeprom_read_config(EEADR_MENU_ITEM(SA));
						if(sa){
#if defined(MINUTE)
							int diff = temperature - setpoint;
#else
							int diff = temperature - eeprom_read_config(EEADR_MENU_ITEM(SP));
#endif
							if(diff < 0){
								diff = -diff;
							} 
							if(sa < 0){
								sa = -sa;
								LATA0 = diff <= sa;
							} else {
								LATA0 = diff >= sa;
							}
						}
					}

					// Run thermostat
					temperature_control();

					// Show temperature if menu is idle
					if(MENU_IDLE){
						if(LATA0 && SHOW_SA_ALARM){
							led_10.raw = LED_S;
							led_1.raw = LED_A;
							led_01.raw = LED_OFF;
						} else {
#if defined(PB2)
							led_e.e_point = !SENSOR_SELECT;
							if(SENSOR_SELECT){
								temperature_to_led(temperature2);
							} else {
								temperature_to_led(temperature);
							}
#else
							temperature_to_led(temperature);
#endif
						}
						SHOW_SA_ALARM = !SHOW_SA_ALARM;
					}
				}
#endif // !OVBSC
			} // End 1 sec section

			// Reset timer flag
			TMR4IF = 0;
		}

		// Reset watchdog
		ClrWdt();
	}
}
