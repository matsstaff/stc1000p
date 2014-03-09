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
 *                     Relay Cool RA4 | 3     18 | RA1/ICSPCLK (Programming header)
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
 *	         * 7       --------    *    --------       * C
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

#define __16f1828
#include "pic14/pic16f1828.h"

#define EEADR_PROFILE_SETPOINT(profile, step)	(profile*19 + step*2)
#define EEADR_PROFILE_DURATION(profile, step)	(profile*19 + step*2 + 1)
#define EEADR_HYSTERESIS						114
#define EEADR_TEMP_CORRECTION					115
#define EEADR_SETPOINT							116
#define EEADR_COOLING_DELAY						117
#define EEADR_CURRENT_STEP						118
#define EEADR_CURRENT_STEP_DURATION				119
#define EEADR_RUN_MODE							120

/* Defines */
#define ClrWdt() { __asm CLRWDT __endasm; }

/* Configuration words */
unsigned int __at _CONFIG1 __CONFIG1 = 0xFDC;
unsigned int __at _CONFIG2 __CONFIG2 = 0x3AFF;

/* Temperature lookup table  */
#ifdef FAHRENHEIT
const int ad_lookup[32] = { -508, -286, -139, -26, 68, 161, 226, 294, 360, 421,
		482, 540, 597, 655, 712, 770, 829, 891, 954, 1020, 1090, 1168, 1243,
		1332, 1431, 1544, 1679, 1855, 2057, 2372, 2957 };
#else  // CELSIUS
const int ad_lookup[32] = {0, -460, -337, -255, -192, -140, -87, -53, -14, 22,
	56, 90, 122, 154, 186, 218, 250, 283, 317, 352, 389, 428, 471, 513, 562,
	617, 680, 755, 853, 965, 1140, 1465};
#endif

/* LED character lookup table (0-15), includes hex */
unsigned const char led_lookup[16] = { 0x3, 0xb7, 0xd, 0x25, 0xb1, 0x61, 0x41,
		0x37, 0x1, 0x21, 0x5, 0xc1, 0xcd, 0x85, 0x9, 0x59 };

/* Global variables to hold LED data (for multiplexing purposes) */
unsigned char led_e=0xff, led_10, led_1, led_01;

/* Declare functions used from Page 1 */
extern unsigned char button_menu_fsm();


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
	    GIE = 0;

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

/* Update LED globals with temperature or integer data.
 * arguments: value (actual temperature multiplied by 10 or an integer)
 *            decimal indicates if the value is multiplied by 10 (i.e. a temperature)
 * return: nothing
 */
void value_to_led(int value, unsigned char decimal) {
	unsigned char i;

	// Handle negative values
	if (value < 0) {
		led_e = led_e & ~(1 << 4);
		value = -value;
	} else {
		led_e = led_e | (1 << 4);
	}

	// If temperature > 100 we must lose decimal...
	if (value >= 1000) {
		value = value / 10;
		decimal = 0;
	}

	// Convert value to BCD and set LED outputs
	if(value >= 100){
		for(i=0; value >= 100; i++){
			value -= 100;
		}
		led_10 = led_lookup[i & 0xf];
	} else {
		led_10 = 0xff; // Turn off led if zero (lose leading zeros)
	}
	if(value >= 10 || decimal){ // If decimal, we want 1 leading zero
		for(i=0; value >= 10; i++){
			value -= 10;
		}
		led_1 = led_lookup[i] & (decimal ? 0xfe : 0xff);
	} else {
		led_1 = 0xff; // Turn off led if zero (lose leading zeros)
	}
	led_01 = led_lookup[value];
}

// Define shortcuts to display LED data. Makes code a bit clearer.
#define int_to_led(v)			value_to_led(v, 0);
#define temperature_to_led(v)	value_to_led(v, 1);

#if FILTER_AD
static int read_temperature(){
	static int ad_acc, ad_min, ad_max, temperature=0;
	static char adcount=0;

	if (!ADGO) { // Remove check? According to data sheet ~10us
		unsigned int adresult = (ADRESH << 8) | ADRESL;

		ADGO = 1; // ADCON0bits.ADGO = 1;

		if(adcount == 0){
			ad_acc = 0;
			ad_max = 0x0;
			ad_min = 0x3FF;
		}

		ad_max = (adresult > ad_max) ? adresult : ad_max;
		ad_min = (adresult < ad_min) ? adresult : ad_min;
		ad_acc += adresult;
		adcount++;

		if(adcount == 10){
			unsigned char i;
			adcount=0;
			ad_acc -= ad_min;
			ad_acc -= ad_max;
			adresult = (ad_acc >> 3);
			temperature = 0;
			for (i = 0; i < 32; i++) {
				if((adresult & 0x1f) <= i){
					temperature += ad_lookup[((adresult >> 5) & 0x1f)];
				} else {
					temperature += ad_lookup[((adresult >> 5) & 0x1f) + 1];
				}
			}
			temperature = (temperature >> 5) + eeprom_read_config(EEADR_TEMP_CORRECTION);
		}
	}
	return temperature;
}
#else

/* Read AD, linearize result and convert to temperature.
 * arguments: nose
 * returns: current temperature
 */
static int read_temperature(){
	int temperature=0;

	if (!ADGO) { // This check should never fail!
		unsigned int adresult = (ADRESH << 8) | ADRESL;
		unsigned char i;

		// Start new conversion
		ADGO = 1;

		// Alarm on sensor error
		if(adresult > 1000 || adresult < 64){
			RA0 = 1;
		} else {
			RA0 = 0;
		}

		// Interpolate between lookup table points
		for (i = 0; i < 32; i++) {
			if((adresult & 0x1f) <= i){
				temperature += ad_lookup[((adresult >> 5) & 0x1f)];
			} else {
				temperature += ad_lookup[((adresult >> 5) & 0x1f) + 1];
			}
		}
		// Divide by 32 and add temperature correction
		temperature = (temperature >> 5) + eeprom_read_config(EEADR_TEMP_CORRECTION);
	}
	return temperature;
}
#endif

/* Thermostat
 * Handles profile and outputs based on temperature.
 * Called every 60ms.
 * arguments: current temperature
 * returns: nothing
 */
static void temperature_control(int temperature){
	static unsigned int cooling_delay = 300;	 // Initial cooling delay
	static unsigned char heating_delay = 180; // Just to spare the relay a bit
	static unsigned int millisx60=0;
	unsigned char mode;
	int setpoint;

	// Only run every 16th time called, that is 16x60ms = 960ms
	// Close enough to 1s for our purposes.
	if(++millisx60 & 0xf){
		return;
	}

	mode = eeprom_read_config(EEADR_RUN_MODE);

	if (mode < 6) { // Running profile
		unsigned char eestep;
		unsigned int eehours;

		// Indicate profile mode
		led_e = led_e & ~(1<<6);

		// Load step and duration
		eestep = eeprom_read_config(EEADR_CURRENT_STEP);
		eestep = eestep > 8 ? 8 : eestep;
		eehours = eeprom_read_config(EEADR_CURRENT_STEP_DURATION);

		// Has it has struck the hour? If so update profile data
		if (millisx60 >= 60000) {
			millisx60 = 0;
			eehours++;
			if (eehours >= eeprom_read_config(EEADR_PROFILE_DURATION(mode, eestep))) {
				eestep++;
				eehours = 0;
				// Is this the last step? Update settings and switch to thermostat mode.
				if (eestep == 9	|| eeprom_read_config(EEADR_PROFILE_DURATION(mode, eestep)) == 0) {
					eeprom_write_config(EEADR_SETPOINT,	eeprom_read_config(EEADR_PROFILE_SETPOINT(mode, eestep)));
					eeprom_write_config(EEADR_RUN_MODE, 6);
					return; // Fastest way out...
				}
				eeprom_write_config(EEADR_CURRENT_STEP, eestep);
#ifndef PROFILE_RAMPING
				eeprom_write_config(EEADR_SETPOINT, eeprom_read_config(EEADR_PROFILE_SETPOINT(mode, eestep)));
			}
#else
			}
			{
				unsigned int eedur = eeprom_read_config(EEADR_PROFILE_DURATION(mode, eestep));
				int sp1 = eeprom_read_config(EEADR_PROFILE_SETPOINT(mode, eestep));
				int sp2 = eeprom_read_config(EEADR_PROFILE_SETPOINT(mode, eestep + 1));
				unsigned int t = (eedur - eehours);
				int sp = ((long) sp1 * t + (long) sp2 * eehours) / eedur;

				eeprom_write_config(EEADR_SETPOINT, sp);
			}
#endif
			eeprom_write_config(EEADR_CURRENT_STEP_DURATION, eehours);
		}
	} else {
		led_e = led_e | (1<<6);
		// Reset counter if switch to profile mode is made
		millisx60 = 0;
	}

	setpoint = eeprom_read_config(EEADR_SETPOINT);

	cooling_delay = (cooling_delay > 0) ? (cooling_delay - 1) : 0;
	heating_delay = (heating_delay > 0) ? (heating_delay - 1) : 0;

	// This is the thermostat logic
	if (LATA4) {
		led_e = led_e & ~(1<<7);
		led_e = led_e | (1<<3);
		if (temperature <= setpoint) {
			cooling_delay = eeprom_read_config(EEADR_COOLING_DELAY) * 60;
			RA4 = 0;
		}
	} else if(LATA5) {
		led_e = led_e & ~(1<<3);
		led_e = led_e | (1<<7);
		if (temperature >= setpoint) {
			heating_delay = 180;
			RA5 = 0;
		}
	} else {
		int hysteresis = eeprom_read_config(EEADR_HYSTERESIS);
		if (temperature >= setpoint + hysteresis) {
			if (cooling_delay) {
				led_e = led_e ^ (1<<7); // Flash to indicate cooling delay
				led_e = led_e | (1<<3);
			} else {
				RA4 = 1;
			}
		} else if (temperature <= setpoint - hysteresis) {
			if (heating_delay) {
				led_e = led_e ^ (1<<3); // Flash to indicate heating delay
				led_e = led_e | (1<<7);
			} else {
				RA5 = 1;
			}
		} else {
			led_e = led_e | (1<<7) | (1<<3);
		}
	}
}

/* Initialize hardware etc, on startup.
 * arguments: none
 * returns: nothing
 */
static void init() {

//   OSCCON = 0b01100010; // 2MHz
	OSCCON = 0b01101010; // 4MHz

	// Heat, cool as output, Thermistor as input, piezo output
	TRISA = 0b00001110;
	PORTA = 0; // Drive relays and piezo low

	// LED Common anodes
	TRISB = 0;
	PORTB = 0;

	// LED data (and buttons) output
	TRISC = 0;

	// Analog input on thermistor
	ANSA2 = 1;
	// Select AD channel AN2
	CHS1 = 1;
	// AD clock FOSC/8 (FOSC = 4MHz)
	ADCS0 = 1;
	// Right justify AD result
	ADFM = 1;
	// Enable AD
	ADON = 1;
	// Start conversion
	ADGO = 1;

	// IMPORTANT FOR BUTTONS TO WORK!!! Disable analog input -> enables digital input
	ANSELC = 0;

	// Postscaler 1:1, Enable counter, prescaler 1:4
	T2CON = 0b00000101;
	// @4MHz, Timer 2 clock is FOSC/4 -> 1MHz prescale 1:4-> 250kHz, 250 gives interrupt every 1 ms
	PR2 = 250;
	// Enable Timer2 interrupt
	TMR2IE = 1;

	// Postscaler 1:15, Enable counter, prescaler 1:16
	T4CON = 0b01110110;
	// @4MHz, Timer 2 clock is FOSC/4 -> 1MHz prescale 1:16-> 62.5kHz, 250 and postscale 1:15 -> 16.66666 Hz or 60ms
	PR4 = 250;

	// Set PEIE (enable peripheral interrupts, that is for timer2) and GIE (enable global interrupts)
	INTCON = 0b11000000;

}

/* Interrupt service routine.
 * Receives timer2 interrupts every millisecond.
 * Handles multiplexing of the LEDs.
 */
static void interrupt_service_routine(void) __interrupt 0 {

	// Check for Timer 2 interrupt
	// Kind of excessive when it's the only enabled interrupt
	// but is nice as reference if more interrupts should be needed
	if (TMR2IF) {
		unsigned char _portb = (LATB << 1);

		if(_portb == 0){
			_portb = 0x10;
		}

		TRISC = 0; // Ensure LED data pins are outputs
		PORTB = 0; // Disable LED's while switching

		// Multiplex LED's every millisecond
		switch(_portb) {
			case 0x10:
			PORTC = led_10;
			break;
			case 0x20:
			PORTC = led_1;
			break;
			case 0x40:
			PORTC = led_01;
			break;
			case 0x80:
			PORTC = led_e;
			break;
		}

		// Enable new LED
		PORTB = _portb;

		// Clear interrupt flag
		TMR2IF = 0;
	}
}

/*
 * Main entry point.
 */
void main(void) __naked {

	init();

	//Loop forever
	while (1) {


		if(TMR4IF){
			int temperature;

			// Temperature control
			temperature = read_temperature();
			temperature_control(temperature);

			// Check buttons and handle menu OR display temperature if idle
			if(button_menu_fsm() == 0){
				temperature_to_led(temperature);
			}

			// Reset timer flag
			TMR4IF = 0;
		}

		// Reset watchdog
		ClrWdt();
	}
}
