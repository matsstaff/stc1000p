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

#define __16f1828
#include "pic14/pic16f1828.h"
#include "stc1000p.h"

/* Defines */
#define ClrWdt() { __asm CLRWDT __endasm; }

/* Configuration words */
unsigned int __at _CONFIG1 __CONFIG1 = 0xFDC;
unsigned int __at _CONFIG2 __CONFIG2 = 0x3AFF;

/* Temperature lookup table  */
#ifdef FAHRENHEIT
const int ad_lookup[32] = { 0, -526, -322, -171, -51, 47, 132, 210, 280, 348, 412, 473, 534, 592, 653, 711, 770, 829, 892, 958, 1024, 1096, 1171, 1253, 1343, 1443, 1558, 1693, 1862, 2075, 2390, 2840 };
#else  // CELSIUS
const int ad_lookup[32] = { 0, -470, -357, -273, -206, -152, -104, -61, -22, 16, 51, 85, 119, 151, 185, 217, 250, 283, 318, 354, 391, 431, 473, 519, 568, 624, 688, 763, 857, 975, 1150, 1400 };
#endif

/* LED character lookup table (0-15), includes hex */
unsigned const char led_lookup[16] = { 0x3, 0xb7, 0xd, 0x25, 0xb1, 0x61, 0x41, 0x37, 0x1, 0x21, 0x5, 0xc1, 0xcd, 0x85, 0x9, 0x59 };

/* Global variables to hold LED data (for multiplexing purposes) */
_led_e_bits led_e = {0xff};
unsigned char led_10, led_1, led_01;

static int temperature=0;

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
		led_e.e_negative = 0;
		value = -value;
	} else {
		led_e.e_negative = 1;
	}

	// If temperature > 100 we must lose decimal...
	if (value >= 1000) {
		value = ((unsigned int) value) / 10;
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
	if(value >= 10 || decimal || led_10!=0xff){ // If decimal, we want 1 leading zero
		for(i=0; value >= 10; i++){
			value -= 10;
		}
		led_1 = led_lookup[i] & (decimal ? 0xfe : 0xff);
	} else {
		led_1 = 0xff; // Turn off led if zero (lose leading zeros)
	}
	led_01 = led_lookup[value];
}


static void update_profile(){
	unsigned char mode;

	mode = eeprom_read_config(EEADR_RUN_MODE);

	if (mode < 6) { // Running profile
		unsigned char eestep;
		unsigned int eehours;

		// Load step and duration
		eestep = eeprom_read_config(EEADR_CURRENT_STEP);
		eestep = eestep > 8 ? 8 : eestep;	// sanity check
		eehours = eeprom_read_config(EEADR_CURRENT_STEP_DURATION);

		eehours++;  // TODO Maybe increment conditionally to be able to call this function outside of on one hour marks?
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
			eeprom_write_config(EEADR_SETPOINT, eeprom_read_config(EEADR_PROFILE_SETPOINT(mode, eestep)));
		}
		eeprom_write_config(EEADR_CURRENT_STEP_DURATION, eehours);
	}
}

/* Due to a fault in SDCC, static local variables are not initialized
 * properly, so the variables below were moved from temperature_control()
 * and made global.
 */
static unsigned int cooling_delay = 60;  // Initial cooling delay (secs)
static unsigned char heating_delay = 60; // Initial heating delay (secs)
static void temperature_control(){
	int setpoint;

	setpoint = eeprom_read_config(EEADR_SETPOINT);

	cooling_delay -= (cooling_delay > 0) ? 1 : 0;
	heating_delay -= (heating_delay > 0) ? 1 : 0;

	// Set LED outputs
	led_e.e_cool = !LATA4;
	led_e.e_heat = !LATA5;

	// This is the thermostat logic
	if (LATA4) {
		if (temperature <= setpoint) {
			cooling_delay = eeprom_read_config(EEADR_COOLING_DELAY) * 60;
			RA4 = 0;
		}
	} else if(LATA5) {
		if (temperature >= setpoint) {
			heating_delay  = eeprom_read_config(EEADR_HEATING_DELAY) * 60;
			RA5 = 0;
		}
	} else {
		int hysteresis = eeprom_read_config(EEADR_HYSTERESIS);
		if (temperature > setpoint + hysteresis) {
			if (cooling_delay) {
				led_e.e_cool = led_e.e_cool ^ (cooling_delay & 0x1); // Flash to indicate cooling delay
			} else {
				RA4 = 1;
			}
		} else if (temperature < setpoint - hysteresis) {
			if (heating_delay) {
				led_e.e_heat = led_e.e_heat ^ (heating_delay & 0x1); // Flash to indicate heating delay
			} else {
				RA5 = 1;
			}
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
			PORTC = led_e.led_e;
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
	unsigned int millisx60=0;

	init();

	//Loop forever
	while (1) {


		if(TMR4IF){

			// Read and accumulate AD value, note at this point temperature variable is not temp, but AD acc
			temperature += (ADRESH << 8) | ADRESL;

			// Start new conversion
			ADGO = 1;

			// Handle button press and menu
			button_menu_fsm();

			// Only run every 16th time called, that is 16x60ms = 960ms
			// Close enough to 1s for our purposes.
			if((++millisx60 & 0xf) == 0){

				temperature >>= 4; // Divide by 16 to get back to a regular AD value

#ifdef WHY_WONT_THIS_WORK
				// Alarm on sensor error (AD result out of range)
				RA0 = (temperature >= 992 || temperature < 32);
#endif
				// Interpolate between lookup table points
				{
					unsigned char i;
					unsigned temp = 0;
					for (i = 0; i < 32; i++) {
						if((temperature & 0x1f) <= i){
							temp += ad_lookup[((temperature >> 5) & 0x1f)];
						} else {
							temp += ad_lookup[((temperature >> 5) & 0x1f) + 1];
						}
					}

					// Divide by 32 and add temperature correction, now temperature actually is temperature (x10)
					temperature = (temp >> 5) + eeprom_read_config(EEADR_TEMP_CORRECTION);
				}

				if(eeprom_read_config(EEADR_POWER_ON) || LATA0){ // Bypass regulation if power is 'off' or alarm

					// Update running profile every hour (if there is one)
					// and handle reset of millis x60 counter
					if(((unsigned char)eeprom_read_config(EEADR_RUN_MODE)) < 6){
						// Indicate profile mode
						led_e.e_set = 0;
						// Update profile every hour
						if(millisx60 == 60000){
							update_profile();
							millisx60 = 0;
						}
					} else {
						led_e.e_set = 1;
						millisx60 = 0;
					}

//					ClrWdt();

					// Run thermostat
					temperature_control();

					// Show temperature if menu is idle
					if(TMR1GE){
						temperature_to_led(temperature);
					}

				} else { // Power is 'off' or alarm, disable outputs
					led_e.led_e = led_10 = led_1 = led_01 = 0xff;
					RA4 = 0;
					RA5 = 0;
				}

				// Reset temperature for A/D acc
				temperature = 0;

			} // End 1 sec section


			// Reset timer flag
			TMR4IF = 0;
		}

		// Reset watchdog
		ClrWdt();
	}
}
