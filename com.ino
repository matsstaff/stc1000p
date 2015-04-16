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

		for(i=2; i<32; i++){
			if(isBlank(cmd[i]) || isEOL(cmd[i])){
				break;
			}
			if(isDigit(cmd[i])){
				if(address>12){
					return error();
				} else {
					address = address * 10 + (cmd[i] - '0');
			 	}
			} else {
				return error();
			}
		}

		if(address > 127){
			return error();
		}

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


