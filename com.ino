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

#define	COM_PIN			9	// ICSPCLK

#define COM_READ_EEPROM		0x20
#define COM_WRITE_EEPROM	0xE0
#define COM_READ_TEMP		0x01
#define COM_ACK			0x9A
#define COM_NACK		0x66

enum set_menu_enum {
	hysteresis,			// hy (hysteresis)
	hysteresis2,			// hy2 (hysteresis probe 2)
	temperature_correction,		// tc (temperature correction)
	temperature_correction2,	// tc2 (temperature correction probe 2)
	setpoint_alarm,			// SA (setpoint alarm)
	setpoint,			// SP (setpoint)
	step,				// St (current running profile step)	
	duration,			// dh (current running profile step duration in hours)
	cooling_delay,			// cd (cooling delay minutes)
	heating_delay,			// hd (heating delay minutes)
	ramping,			// rP (0=disable, 1=enable ramping)
	probe2,				// Pb (0=disable, 1=enable probe2 for regulation)
	run_mode			// rn (0-5 run profile, 6=thermostat)
};

/* Defines for EEPROM config addresses */
#define EEADR_PROFILE_SETPOINT(profile, stp)	(((profile)*19) + ((stp)<<1))
#define EEADR_PROFILE_DURATION(profile, stp)	(EEADR_PROFILE_SETPOINT(profile, stp) + 1)
#define EEADR_SET_MENU				EEADR_PROFILE_SETPOINT(6, 0)
#define EEADR_SET_MENU_ITEM(name)		(EEADR_SET_MENU + (name))
#define EEADR_POWER_ON				127

void write_bit(unsigned const char data){
	pinMode(COM_PIN, OUTPUT);
	digitalWrite(COM_PIN, HIGH);
	delayMicroseconds(7);
	if(!data){
		pinMode(COM_PIN, INPUT);
		digitalWrite(COM_PIN, LOW);
	}
	delayMicroseconds(400);
	pinMode(COM_PIN, INPUT);
	digitalWrite(COM_PIN, LOW);
	delayMicroseconds(100);
}

unsigned char read_bit(){
	unsigned char data;

	pinMode(COM_PIN, OUTPUT);
	digitalWrite(COM_PIN, HIGH);
	delayMicroseconds(7);
	pinMode(COM_PIN, INPUT);
	digitalWrite(COM_PIN, LOW);
	delayMicroseconds(200);
	data = digitalRead(COM_PIN);
	delayMicroseconds(300);

	return data;
}

void write_byte(unsigned const char data){
	unsigned char i;
	
	for(i=0;i<8;i++){
		write_bit(((data << i) & 0x80));
	}
	delayMicroseconds(500);
}

unsigned char read_byte(){
	unsigned char i, data;
	
	for(i=0;i<8;i++){
		data <<= 1;
		if(read_bit()){
			data |= 1;
		}
	}
	delayMicroseconds(500);

	return data;
}


bool write_eeprom(const unsigned char address, unsigned const int value){
	unsigned char ack;
	write_byte(COM_WRITE_EEPROM);
	write_byte(address);
	write_byte(((unsigned char)(value >> 8)));
	write_byte((unsigned char)value);
	write_byte(COM_WRITE_EEPROM ^ address ^ ((unsigned char)(value >> 8)) ^ ((unsigned char)value));
	delay(6); // Longer delay needed here for EEPROM write to finish, but must be shorter than 10ms
	ack = read_byte();
	return ack == COM_ACK; 
}

bool read_eeprom(const unsigned char address, int *value){
	unsigned char xorsum;
	unsigned char ack;
        unsigned int data;

	write_byte(COM_READ_EEPROM);
	write_byte(address);
	data = read_byte();
	data = (data << 8) | read_byte();
	xorsum = read_byte();
        ack = read_byte();
	if(ack == COM_ACK && xorsum == (COM_READ_EEPROM ^ address ^ ((unsigned char)(data >> 8)) ^ ((unsigned char)data))){
	        *value = (int)data;
		return true;
	}
	return false;
}

bool read_temp(int *temperature){
	unsigned char xorsum;
	unsigned char ack;
        unsigned int data;

	write_byte(COM_READ_TEMP);
	data = read_byte();
	data = (data << 8) | read_byte();
	xorsum = read_byte();
        ack = read_byte();
	if(ack == COM_ACK && xorsum == (COM_READ_TEMP ^ ((unsigned char)(data >> 8)) ^ ((unsigned char)data))){
	        *temperature = (int)data;
		return true;
	}
	return false;
}

/* End of communication implementation */

/* From here example implementation begins, this can be exchanged for your specific needs */

#if 0
    _(hy, 	LED_h, 	LED_y, 	LED_OFF, 	0, 		TEMP_HYST_1_MAX,	5,		10) 	\
    _(hy2, 	LED_h, 	LED_y, 	LED_2, 		0, 		TEMP_HYST_2_MAX, 	50,		100)	\
    _(tc, 	LED_t, 	LED_c, 	LED_OFF, 	TEMP_CORR_MIN, 	TEMP_CORR_MAX,		0,		0)	\
    _(tc2, 	LED_t, 	LED_c, 	LED_2, 		TEMP_CORR_MIN,	TEMP_CORR_MAX,		0,		0)	\
    _(SA, 	LED_S, 	LED_A, 	LED_OFF, 	SP_ALARM_MIN,	SP_ALARM_MAX,		0,		0)	\
    _(SP, 	LED_S, 	LED_P, 	LED_OFF, 	TEMP_MIN,	TEMP_MAX,		200,		680)	\
    _(St, 	LED_S, 	LED_t, 	LED_OFF, 	0,		8,			0,		0)	\
    _(dh, 	LED_d, 	LED_h, 	LED_OFF, 	0,		999,			0,		0)	\
    _(cd, 	LED_c, 	LED_d, 	LED_OFF, 	0,		60,			5,		5)	\
    _(hd, 	LED_h, 	LED_d, 	LED_OFF, 	0,		60,			2,		2)	\
    _(rP, 	LED_r, 	LED_P, 	LED_OFF, 	0,		1,			0,		0)	\
    _(Pb, 	LED_P, 	LED_b, 	LED_2, 		0,		1,			0,		0)	\
    _(rn, 	LED_r, 	LED_n, 	LED_OFF, 	0,		6,			6,		6) 	\

#endif

const char menu_opt[][4] = {
	"hy",
	"hy2",
	"tc",
	"tc2",
	"SA",
	"SP",
	"St",
	"dh",
	"cd",
	"hd",
	"rP",
	"Pb",
	"rn"
};

bool isBlank(char c){
	return c == ' ' || c == '\t';
}

bool isDigit(char c){
	return c >= '0' && c <= '9';
}

bool isEOL(char c){
	return c == '\r' || c == '\n';
}

void error(){
	Serial.println("?Syntax error");
}

unsigned char parse_address(const char *cmd, unsigned char *addr){
	unsigned char i;	

	if(!strncmp("SP", cmd, 2)){
		if(isDigit(cmd[2]) && isDigit(cmd[3]) && cmd[2] < '6'){
			*addr = EEADR_PROFILE_SETPOINT(cmd[2]-'0', cmd[3]-'0');
			return 4;
		}
	}

	if(!strncmp("dh", cmd, 2)){
		if(isDigit(cmd[2]) && isDigit(cmd[3]) && cmd[2] < '6'){
			*addr = EEADR_PROFILE_DURATION(cmd[2]-'0', cmd[3]-'0');
			return 4;
		}
	}

	for(i=0; i< (sizeof(menu_opt)/sizeof(menu_opt[0])); i++){
		if(!strncmp(cmd, &menu_opt[i][0], strlen(menu_opt[i]))){
			*addr = EEADR_SET_MENU + i;
			return strlen(menu_opt[i]);
		}
	}

	*addr = 0;
	for(i=0; i<30; i++){
		if(isBlank(cmd[i]) || isEOL(cmd[i])){
			break;
		}
		if(isDigit(cmd[i])){
			if(*addr>12){
				error();
				return 0;
			} else {
				*addr = *addr * 10 + (cmd[i] - '0');
		 	}
		} else {
			error();
			return 0;
		}
	}

	if(*addr > 127){
		return 0;
	}

	return i;
}

void parse_command(char *cmd){
	int data;

	if(cmd[0] == 't'){
		if(read_temp(&data)){
			Serial.print("T:");
			Serial.print(data/10.0);
			Serial.println();
		} else {
			Serial.println("Communication error");
		}
	} else if(cmd[0] == 'r' || cmd[0] == 'w') {
		unsigned char address=0;
		unsigned char i;
		bool neg = false;

		if(!isBlank(cmd[1])){
			return error();
		}

		i = parse_address(&cmd[2], &address);

		if(i==0){
			return error();
		}

		i+=2;

		if(cmd[0] == 'r'){
			if(read_eeprom(address, &data)){
				Serial.print("EEPROM[");
				Serial.print(address);
				Serial.print("]=");
				Serial.println(data);
			} else {
				Serial.println("Communication error");
			}
			return;
		}

		if(!isBlank(cmd[i])){
			return error();
		}
		i++;

		if(cmd[i] == '-'){
			neg = true;
			i++;
		}

		if(!isDigit(cmd[i])){
			return error();
		}

		for(data=0; i<32; i++){
			if(isEOL(cmd[i])){
				break;
			}
			if(isDigit(cmd[i]) && data < 3276){
				data = data * 10 + (cmd[i] - '0');
			} else {
				return error();
			}
		}

		if((neg && data > 32768) || (!neg && data > 32767)){
			return error();
		}

		if(write_eeprom(address, data)){
			Serial.println("Ok");
		} else {
			Serial.println("Communication error");
		}
	}
		
}

void setup() {
	Serial.begin(115200);

	delay(2);

	Serial.println("STC-1000+ communication sketch.");
	Serial.println("Copyright 2015 Mats Staffansson");
	Serial.println("");
	Serial.println("Commands: 't' to read temperature");
	Serial.println("          'r [addr (0-127)]' to read EEPROM address");
	Serial.println("          'w [addr (0-127)] [data]' to write EEPROM address");

}

void loop() {
	static char cmd[32], rxchar=' ';
	static unsigned char index=0; 	

	if(Serial.available() > 0){
		char c = Serial.read();
		if(!(isBlank(rxchar) && isBlank(c))){
			cmd[index] = c;
			rxchar = c;
			index++;
		}

		if(index>=31 || isEOL(rxchar)){
			cmd[index] = '\0';
			parse_command(cmd);
			index = 0;
			rxchar = ' ';
		}
	}

}


