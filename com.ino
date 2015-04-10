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

/* Pin configuration */
#define ICSPCLK 9
#define ICSPDAT 8 
#define VDD1    6
#define VDD2    5
#define VDD3    4
#define nMCLR   3 

#define	COM_PIN	9	// ICSPCLK

#define COM_HANDSHAKE	0xC5
#define COM_ACK		0x9A



void setup() {
	pinMode(ICSPCLK, INPUT);
	digitalWrite(ICSPCLK, LOW); // Disable pull-up
	pinMode(ICSPDAT, INPUT);
	digitalWrite(ICSPDAT, LOW); // Disable pull-up
	pinMode(nMCLR, INPUT);
	digitalWrite(nMCLR, LOW); // Disable pull-up

	Serial.begin(115200);

	delay(2);

	Serial.println("STC-1000+ communication sketch.");
	Serial.println("Copyright 2014 Mats Staffansson");
	Serial.println("");
	Serial.println("Send 'd' to check for STC-1000");

}

void write_byte(const unsigned char data){
	unsigned char i;
	
	pinMode(COM_PIN, OUTPUT);
	digitalWrite(COM_PIN, LOW);

	for(i=0;i<8;i++){
		digitalWrite(COM_PIN, HIGH);
		delayMicroseconds(1);
		if(((data << i) & 0x80) == 0){
			digitalWrite(COM_PIN, LOW);
		}
		delayMicroseconds(300);
		digitalWrite(COM_PIN, LOW);
		delayMicroseconds(100);		
	}
	pinMode(COM_PIN, INPUT);
	delayMicroseconds(500);
}

unsigned char read_byte(){
	unsigned char i, data;
	
	for(i=0;i<8;i++){
		pinMode(COM_PIN, OUTPUT);
		digitalWrite(COM_PIN, HIGH);
		delayMicroseconds(1);
		pinMode(COM_PIN, INPUT);
		digitalWrite(COM_PIN, LOW);
		delayMicroseconds(100);
		data = (data << 1) | digitalRead(COM_PIN);
		delayMicroseconds(300);		
	}
	delayMicroseconds(500);
}


bool write_address(const unsigned char address, const int data){
	write_byte(COM_HANDSHAKE);
	write_byte(address | 0x80);
	write_byte(((unsigned char)(data >> 8)));
	write_byte((unsigned char)data);
	write_byte((address | 0x80) ^ ((unsigned char)(data >> 8)) ^ ((unsigned char)data));
	return read_byte() == COM_ACK; 
}

bool read_address(const unsigned char address, int *data){
	unsigned char xorsum;
	write_byte(COM_HANDSHAKE);
	write_byte(address & 0x7f);
	*data = read_byte();
	*data = (*data << 8) | read_byte();
	xorsum = read_byte();
	return read_byte() == COM_ACK && xorsum == (address ^ ((unsigned char)(*data >> 8)) ^ ((unsigned char)*data)); 
}


void loop() {
	int data;

	if (Serial.available() > 0) {
		char command = Serial.read();
		switch (command) {
		case 't':
			if(read_address(125, &data)){
				Serial.print("T:");
				Serial.print(data);
				Serial.println();
			} else {
				Serial.println("Error");
			}
			break;
		default:
			break;
		}
	}
}

