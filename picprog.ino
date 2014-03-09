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
 * The idea of an Arduino based PIC programmer came from here
 * http://forum.arduino.cc/index.php?topic=92929.0
 * but was completely rewritten.
 *
 * This sketch allows the generated HEX file for the STC-1000 to be uploaded
 * from the Arduino by sending it over serial. I use CuteCom under linux to upload the
 * file, which works wonderfully. It allows to set a character delay, to not overrun the
 * buffer. If you should use another software that does not have this feature you may
 * need to lower the baudrate significantly.
 *
 */

/* Pin configuration */
#define ICSPCLK 9
#define ICSPDAT 8 
#define VDD1    6
#define VDD2    5
#define VDD3    4
#define nMCLR   3 

/* Delays */
#define TDLY()  delayMicroseconds(1)    /* 1.0us minimum */
#define TCKH()  delayMicroseconds(1)    /* 100ns minimum */
#define TCKL()  delayMicroseconds(1)    /* 100ns minimum */
#define TERAB() delay(5)    		/* 5ms maximum */
#define TERAR() delay(3)    		/* 2.5ms maximum */
#define TPINT() delay(5)    		/* 5ms maximum */
#define TPEXT() delay(2)                /* 1.0ms minimum, 2.1ms maximum*/
#define TDIS()  delayMicroseconds(1)    /* 300ns minimum */
#define TENTS() delayMicroseconds(1)    /* 100ns minimum */
#define TENTH() delay(3)  		/* 250us minimum */ /* Needs a lot more when powered by arduino */
#define TEXIT() delayMicroseconds(1)    /* 1us minimum */

/* Commands */
#define LOAD_CONFIGURATION                  0x00    /* 0, data(14), 0 */
#define LOAD_DATA_FOR_PROGRAM_MEMORY        0x02    /* 0, data(14), 0 */
#define LOAD_DATA_FOR_DATA_MEMORY           0x03    /* 0, data(14), zero(6), 0 */
#define READ_DATA_FROM_PROGRAM_MEMORY       0x04    /* 0, data(14), 0 */
#define READ_DATA_FROM_DATA_MEMORY          0x05    /* 0, data(14), zero(6), 0 */
#define INCREMENT_ADDRESS                   0x06    /* - */
#define RESET_ADDRESS                       0x16    /* - */
#define BEGIN_INTERNALLY_TIMED_PROGRAMMING  0x08    /* - */
#define BEGIN_EXTERNALLY_TIMED_PROGRAMMING  0x18    /* - */
#define END_EXTERNALLY_TIMED_PROGRAMMING    0x0A    /* - */
#define BULK_ERASE_PROGRAM_MEMORY           0x09    /* Internally Timed */
#define BULK_ERASE_DATA_MEMORY              0x0B    /* Internally Timed */
#define ROW_ERASE_PROGRAM_MEMORY            0x11    /* Internally Timed */

void setup() {                
  pinMode(ICSPDAT, OUTPUT);   
  digitalWrite(ICSPDAT, LOW);   
  Serial.begin(115200);
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    switch(command)
    {
    case 'h':
      hvp_entry();
      break;
    case 'l':
      lvp_entry();
      break;
    case 'e':
      p_exit();
      break;
    case 'c':
      load_configuration(0);
      break;
    case 'p':
      Serial.print('0');
      Serial.print('x');
      Serial.print(read_data_from_program_memory(), HEX);
      Serial.println();
      break;
    case 'i':
      increment_address();
      break;
    case 'u':
      upload_hex_file_to_device();
      break;
    default:
      break;
    }
  }
}

unsigned char hex_nibble(unsigned char data){
  data = toupper(data);
  return (data >= 'A' ? data-'A'+10: data -'0') & 0xf;
}

unsigned char parse_hex(){
  unsigned char data;
  while(Serial.available() < 2);
  data = hex_nibble(Serial.read()) << 4;
  data |= hex_nibble(Serial.read());

  return data;
}

void upload_hex_file_to_device(){
  unsigned int device_address = 0;
  
  Serial.println("Enter low voltage programming mode");
  lvp_entry();
  Serial.println("Erasing device");
  bulk_erase_device();
  Serial.println("Waiting for hex data...");

  while(1){
    unsigned char bytecount;
    unsigned int address;
    unsigned char recordtype;
    unsigned char data[16];
    unsigned char checksum;
    unsigned char i;

    // Read start of line
    while(1){
      while(Serial.available() < 1);
      char rx = Serial.read();
      if(rx == ':'){
        break;
      }
    }

    // Read bytecount
    bytecount = parse_hex();
    checksum = bytecount;

    // Read address
    address = ((unsigned int)parse_hex()) << 8;
    address |= parse_hex();
    checksum += ((unsigned char)(address>>8));
    checksum += ((unsigned char)(address));

    // Read recordtype
    recordtype = parse_hex();
    checksum += recordtype;

    for(i=0; i<bytecount; i++){
      data[i] = parse_hex();
      checksum += data[i];
    }

    // Read checksum
    i = parse_hex();
    checksum += i;

    if(checksum){
      Serial.println("Checksum error!");
      break;
    }

    if(recordtype == 1){
      Serial.println("Programming done");
      break;
    } else if(recordtype == 04){
      if(data[1] == 0){
        Serial.println("Programming program memory");
        reset_address();
        device_address = 0;
      } else if(data[1] == 1){
        Serial.println("Programming config memory");
        load_configuration(0);
        device_address = 0;
      }
    } else if(recordtype == 00) {
      Serial.print("Programming ");
      Serial.print(bytecount>>1, DEC);
      Serial.print(" words at address 0x");
      Serial.println(address >> 1, HEX);
      
      while(device_address != (address >> 1)){
        increment_address();
        device_address++;
      }
      
      for(i=0; i<bytecount; i+=2){
        unsigned int data_word_out = (((unsigned int)data[i+1]) << 8) | data[i];
        unsigned int data_word_in;
        load_data_for_program_memory(data_word_out);
        begin_internally_timed_programming();
        data_word_in = read_data_from_program_memory();
        if(data_word_in != data_word_out){
          Serial.print("Validation failed for address 0x");
          Serial.print(device_address, HEX);
          Serial.print(" wrote 0x");
          Serial.print(data_word_out, HEX);
          Serial.print(" but read back 0x");
          Serial.println(data_word_in, HEX);
        }
        increment_address();
        device_address++;
      }
    }
  }
  Serial.println("Leaving programming mode");
  p_exit();
}

/* low level bit transfer */
void write_bit(unsigned char bit){
  digitalWrite(ICSPCLK, HIGH);
  digitalWrite(ICSPDAT, bit ? HIGH : LOW);
  TCKH();
  digitalWrite(ICSPCLK, LOW);
  TCKL();
  //  digitalWrite(ICSPDAT,LOW); // REM?
}

unsigned char read_bit(){
  unsigned char rv;

  digitalWrite(ICSPCLK, HIGH);
  TCKH();
  rv = (digitalRead(ICSPDAT)==HIGH);
  digitalWrite(ICSPCLK, LOW);
  TCKL();

  return rv;
}

/* Program/verify mode entry and exit */
void hvp_entry(){
  pinMode(ICSPCLK, OUTPUT);    
  pinMode(VDD1, OUTPUT);
  pinMode(VDD2, OUTPUT);
  pinMode(VDD3, OUTPUT);
  pinMode(ICSPDAT, OUTPUT);   
  // Set VPP to VIHH (9v)

  TENTS();
  digitalWrite(VDD1, HIGH);
  digitalWrite(VDD2, HIGH);
  digitalWrite(VDD3, HIGH);
  TENTH();
}

void lvp_entry(){
  unsigned long LVP_magic = 0b01001101010000110100100001010000;
  unsigned char i;

  pinMode(ICSPCLK, OUTPUT);    
  pinMode(VDD1, OUTPUT);
  pinMode(VDD2, OUTPUT);
  pinMode(VDD3, OUTPUT);
  pinMode(ICSPDAT, OUTPUT);   
  pinMode(nMCLR, OUTPUT);

  digitalWrite(nMCLR, HIGH);
  digitalWrite(VDD1, HIGH);
  digitalWrite(VDD2, HIGH);
  digitalWrite(VDD3, HIGH);
  TENTS();
  digitalWrite(nMCLR, LOW);
  TENTH();

  // Send "MCHP" backwards, to unlock LVP mode 
  for(i=0; i<32; i++){
    write_bit((LVP_magic >> i) & 1);
  }
  write_bit(0);
}

void p_exit(){
  digitalWrite(nMCLR, LOW); // LVP mode
  digitalWrite(ICSPCLK, LOW);
  digitalWrite(ICSPDAT, LOW);
  digitalWrite(VDD1, LOW);
  digitalWrite(VDD2, LOW);
  digitalWrite(VDD3, LOW);
  TEXIT();

  pinMode(nMCLR, INPUT); // LVP mode
//  pinMode(ICSPCLK, INPUT);    
  pinMode(VDD1, INPUT);
  pinMode(VDD2, INPUT);
  pinMode(VDD3, INPUT);
  pinMode(ICSPDAT, INPUT);   
}

/* low level command transfer */
void write_command(unsigned char command){
  unsigned char i;

  for(i=0; i<5; i++){
    write_bit((command >> i) & 1);
  }
  write_bit(0);
  TDLY();
}

void write_command_with_data(unsigned char command, unsigned int data){
  unsigned char i;

  write_command(command);

  write_bit(0);
  for(i=0; i<14; i++){
    write_bit((data>>i) & 1);
  }
  write_bit(0);
}

unsigned int read_command(unsigned char command){
  unsigned char i;
  unsigned int data = 0;

  write_command(command);

  pinMode(ICSPDAT, INPUT);

  read_bit();
  for(i=0; i<14; i++){
    data |= ((unsigned int)(read_bit() & 0x1) << i);
  }
  read_bit();

  pinMode(ICSPDAT, OUTPUT);

  return data;
}

/* high level commands */
void load_configuration(unsigned int data){
  write_command_with_data(LOAD_CONFIGURATION, data);
}

void load_data_for_program_memory(unsigned int data){
  write_command_with_data(LOAD_DATA_FOR_PROGRAM_MEMORY, data);
}

void load_data_for_data_memory(unsigned char data){
  write_command_with_data(LOAD_DATA_FOR_DATA_MEMORY, data);
}

unsigned int read_data_from_program_memory(){
  return read_command(READ_DATA_FROM_PROGRAM_MEMORY);
}

unsigned char read_data_from_data_memory(){
  return (unsigned char)read_command(READ_DATA_FROM_PROGRAM_MEMORY);
}

void increment_address(){
  write_command(INCREMENT_ADDRESS);
}

void reset_address(){
  write_command(RESET_ADDRESS);
}

void begin_internally_timed_programming(){
  write_command(BEGIN_INTERNALLY_TIMED_PROGRAMMING);
  TPINT();
}

void begin_externally_timed_programming(){
  write_command(BEGIN_EXTERNALLY_TIMED_PROGRAMMING);
  TPEXT();
}

void end_externally_timed_programming(){
  write_command(END_EXTERNALLY_TIMED_PROGRAMMING);
  TDIS();
}

void bulk_erase_program_memory(){
  write_command(BULK_ERASE_PROGRAM_MEMORY);
  TERAB();
}

void bulk_erase_data_memory(){
  write_command(BULK_ERASE_DATA_MEMORY);
  TERAB();
}

void row_erase_program_memory(){
  write_command(ROW_ERASE_PROGRAM_MEMORY);
  TERAR();
}

/* algorithms */
void bulk_erase_device(){
  load_configuration(0);
  bulk_erase_program_memory();
  bulk_erase_data_memory();
}


