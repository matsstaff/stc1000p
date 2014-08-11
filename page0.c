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

#define __16f1828
#include "pic14/pic16f1828.h"
#include "stc1000p.h"

/* Defines */
#define ClrWdt() { __asm CLRWDT __endasm; }

/* Configuration words */
unsigned int __at _CONFIG1 __CONFIG1 = 0xFD4;
unsigned int __at _CONFIG2 __CONFIG2 = 0x3AFF;

/* Temperature lookup table  */
#ifdef FAHRENHEIT
const int ad_lookup[] = { 0, -555, -319, -167, -49, 48, 134, 211, 282, 348, 412, 474, 534, 593, 652, 711, 770, 831, 893, 957, 1025, 1096, 1172, 1253, 1343, 1444, 1559, 1694, 1860, 2078, 2397, 2987 };
#else  // CELSIUS
const int ad_lookup[] = { 0, -486, -355, -270, -205, -151, -104, -61, -21, 16, 51, 85, 119, 152, 184, 217, 250, 284, 318, 354, 391, 431, 473, 519, 569, 624, 688, 763, 856, 977, 1154, 1482 };
#endif

/* LED character lookup table (0-15), includes hex */
//unsigned const char led_lookup[] = { 0x3, 0xb7, 0xd, 0x25, 0xb1, 0x61, 0x41, 0x37, 0x1, 0x21, 0x5, 0xc1, 0xcd, 0x85, 0x9, 0x59 };
/* LED character lookup table (0-9) */
unsigned const char led_lookup[] = { 0x3, 0xb7, 0xd, 0x25, 0xb1, 0x61, 0x41, 0x37, 0x1, 0x21 };

/* Global variables to hold LED data (for multiplexing purposes) */
_led_e_bits led_e = {0xff};
unsigned char led_10, led_1, led_01;

static int temperature=0;
static int temperature2=0;

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

	// This assumes that only temperatures and all temperatures are decimal
	if(decimal){
		led_e.e_deg = 0;
#ifdef FAHRENHEIT
		led_e.e_c = 1;
#else
		led_e.e_c = 0;
#endif // FAHRENHEIT
	}

	// If temperature >= 100 we must lose decimal...
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
		led_1 = led_lookup[i];
		if(decimal){
			led_1 &= 0xfe;
		}
	} else {
		led_1 = 0xff; // Turn off led if zero (lose leading zeros)
	}
	led_01 = led_lookup[value];
}

/* To be called once every hour on the hour.
 * Updates EEPROM configuration when running profile.
 */
static void update_profile(){
	unsigned char profile_no = eeprom_read_config(EEADR_RUN_MODE);

	// Running profile?
	if (profile_no < 6) {
		unsigned char curr_step = eeprom_read_config(EEADR_CURRENT_STEP);
		unsigned int curr_dur = eeprom_read_config(EEADR_CURRENT_STEP_DURATION) + 1;
		unsigned char profile_step_eeaddr;
		unsigned int profile_step_dur;
		int profile_next_step_sp;

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
			eeprom_write_config(EEADR_SETPOINT,	profile_next_step_sp);
			// Is this the last step (next step is number 9 or next step duration is 0)?
			if (curr_step == 8 || eeprom_read_config(profile_step_eeaddr + 3) == 0) {
				// Switch to thermostat mode.
				eeprom_write_config(EEADR_RUN_MODE, 6);
				return; // Fastest way out...
			}
			// Reset duration
			curr_dur = 0;
			// Update step
			curr_step++;
			eeprom_write_config(EEADR_CURRENT_STEP, curr_step);
		} else if(eeprom_read_config(EEADR_RAMPING)) { // Is ramping enabled?
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
			eeprom_write_config(EEADR_SETPOINT, sp);
		}
		// Update duration
		eeprom_write_config(EEADR_CURRENT_STEP_DURATION, curr_dur);
	}
}

/* Due to a fault in SDCC, static local variables are not initialized
 * properly, so the variables below were moved from temperature_control()
 * and made global.
 */
static unsigned int cooling_delay = 60;  // Initial cooling delay
static unsigned int heating_delay = 60;  // Initial heating delay
static void temperature_control(){
	int setpoint = eeprom_read_config(EEADR_SETPOINT);
	int hysteresis2 = eeprom_read_config(EEADR_HYSTERESIS_2);
	unsigned char probe2 = eeprom_read_config(EEADR_2ND_PROBE);

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
	if((LATA4 && (temperature <= setpoint || (probe2 && (temperature2 < (setpoint - hysteresis2))))) || (LATA5 && (temperature >= setpoint || (probe2 && (temperature2 > (setpoint + hysteresis2)))))){
		cooling_delay = eeprom_read_config(EEADR_COOLING_DELAY) << 6;
		cooling_delay = cooling_delay - (cooling_delay >> 4);
		heating_delay = eeprom_read_config(EEADR_HEATING_DELAY) << 6;
		heating_delay = heating_delay - (heating_delay >> 4);
		LATA4 = 0;
		LATA5 = 0;
	}
	else if(LATA4 == 0 && LATA5 == 0) {
		int hysteresis = eeprom_read_config(EEADR_HYSTERESIS);
		hysteresis2 >>= 2; // Halve hysteresis 2
		if ((temperature > setpoint + hysteresis) && (!probe2 || (temperature2 >= setpoint - hysteresis2))) {
			if (cooling_delay) {
				led_e.e_cool = led_e.e_cool ^ (cooling_delay & 0x1); // Flash to indicate cooling delay
			} else {
				LATA4 = 1;
			}
		} else if ((temperature < setpoint - hysteresis) && (!probe2 || (temperature2 <= setpoint + hysteresis2))) {
			if (heating_delay) {
				led_e.e_heat = led_e.e_heat ^ (heating_delay & 0x1); // Flash to indicate heating delay
			} else {
				LATA5 = 1;
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
	LATA = 0; // Drive relays and piezo low

	// LED Common anodes
	TRISB = 0;
	LATB = 0;

	// LED data (and buttons) output
	TRISC = 0;

	// Analog input on thermistor
	ANSELA = _ANSA1 | _ANSA2;
	// Select AD channel AN2
//	ADCON0bits.CHS = 2;
	// AD clock FOSC/8 (FOSC = 4MHz)
	ADCS0 = 1;
	// Right justify AD result
	ADFM = 1;
	// Enable AD
//	ADON = 1;
	// Start conversion
//	ADGO = 1;

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
	TMR4ON = eeprom_read_config(EEADR_POWER_ON);
	// @4MHz, Timer 2 clock is FOSC/4 -> 1MHz prescale 1:16-> 62.5kHz, 250 and postscale 1:15 -> 16.66666 Hz or 60ms
	PR4 = 250;

	// Postscaler 1:7, Enable counter, prescaler 1:64
	T6CON = 0b00110111;
	// @4MHz, Timer 2 clock is FOSC/4 -> 1MHz prescale 1:64-> 15.625kHz, 250 and postscale 1:6 -> 8.93Hz or 112ms
	PR6 = 250;

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
		unsigned char latb = (LATB << 1);

		if(latb == 0){
			latb = 0x10;
		}

		TRISC = 0; // Ensure LED data pins are outputs
		LATB = 0; // Disable LED's while switching

		// Multiplex LED's every millisecond
		switch(latb) {
			case 0x10:
			LATC = led_10;
			break;
			case 0x20:
			LATC = led_1;
			break;
			case 0x40:
			LATC = led_01;
			break;
			case 0x80:
			LATC = led_e.led_e;
			break;
		}

		// Enable new LED
		LATB = latb;

		// Clear interrupt flag
		TMR2IF = 0;
	}
}

#define START_TCONV_1()		(ADCON0 = _CHS1 | _ADON)
#define START_TCONV_2()		(ADCON0 = _CHS0 | _ADON)

static unsigned int read_ad(unsigned int adfilter){
	unsigned char i;
//	for(i=0; i<40; i++){ __asm NOP __endasm; };
	ADGO = 1;
	while(ADGO);
	ADON = 0;
	return ((adfilter - (adfilter >> 6)) + ((ADRESH << 8) | ADRESL));
}

static int ad_to_temp(unsigned int adfilter){
	unsigned char i;
	long temp = 32;
	unsigned char a = ((adfilter >> 5) & 0x3f); // Lower 6 bits
	unsigned char b = ((adfilter >> 11) & 0x1f); // Upper 5 bits

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

/*
 * Main entry point.
 */
void main(void) __naked {
	unsigned int millisx60=0;
	unsigned int ad_filter=0x7fff, ad_filter2=0x7fff;

	init();

	START_TCONV_1();

	//Loop forever
	while (1) {

		if(TMR6IF) {

			// Handle button press and menu
			button_menu_fsm();

			if(!TMR4ON){
				led_e.led_e = 0xff;
				led_10 = 0x3;
				led_1 = led_01 = 0x59;
			}

			// Reset timer flag
			TMR6IF = 0;
		}

		if(TMR4IF) {

			millisx60++;

			if(millisx60 & 0x1){
				ad_filter = read_ad(ad_filter);
				START_TCONV_2();
			} else {
				ad_filter2 = read_ad(ad_filter2);
				START_TCONV_1();
			}

			// Only run every 16th time called, that is 16x60ms = 960ms
			// Close enough to 1s for our purposes.
			if((millisx60 & 0xf) == 0) {

				temperature = ad_to_temp(ad_filter) + eeprom_read_config(EEADR_TEMP_CORRECTION);
				temperature2 = ad_to_temp(ad_filter2) + eeprom_read_config(EEADR_TEMP_CORRECTION_2);

				// Alarm on sensor error (AD result out of range)
				LATA0 = ((ad_filter>>8) >= 248 || (ad_filter>>8) <= 8) || (eeprom_read_config(EEADR_2ND_PROBE) && ((ad_filter2>>8) >= 248 || (ad_filter2>>8) <= 8));

				if(LATA0){ // On alarm, disable outputs
					led_10 = 0x11; // A
					led_1 = 0xcb; //L
					led_e.led_e = led_01 = 0xff;
					LATA4 = 0;
					LATA5 = 0;
					cooling_delay = heating_delay = 60;
				} else {
					// Update running profile every hour (if there is one)
					// and handle reset of millis x60 counter
					if(((unsigned char)eeprom_read_config(EEADR_RUN_MODE)) < 6){
						// Indicate profile mode
						led_e.e_set = 0;
						// Update profile every hour
						if(millisx60 >= 60000){
							update_profile();
							millisx60 = 0;
						}
					} else {
						led_e.e_set = 1;
						millisx60 = 0;
					}

					// Run thermostat
					temperature_control();

					// Show temperature if menu is idle
					if(TMR1GE){
						led_e.e_point = !TX9;
						if(TX9){
							temperature_to_led(temperature2);
						} else {
							temperature_to_led(temperature);
						}
					}
				}

			} // End 1 sec section

			// Reset timer flag
			TMR4IF = 0;
		}

		// Reset watchdog
		ClrWdt();
	}
}
