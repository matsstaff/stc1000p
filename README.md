stc1000+
========

Firmware mod and Arduino based uploader for STC-1000 dual stage thermostat.

The STC-1000 is a dual stage (heating and cooling) thermostat that is pretty cheap to buy. I (and a low of fellow homebrewers) use them to control the fermentation temperature of beer.

The goal of this project is twofold
 * To create the means to reprogram the STC-1000 with a custom firmware
 * To create the custom firmware, suited for fermenting beer

The first goal is achieved by using an Arduino UNO with a sketch to act as a programmer, and the second by using the SDCC compiler and GPUTILS. 

A word of caution though. This project is in its infacy yet, and as the original firmware is protected, there is no way to restore the original functionality. And I assume no responsibilities.

To reprogram the STC-1000, make sure to UNPLUG (not just power off) STC-1000. Connect the necessary wires to the Arduino (see image),  download the sketch and suitible HEX file. Upload the sketch using Arduino IDE. Connect with a serial terminal emulator (115200 bps). I use CuteCom under GNU/Linux. It allows to set a delay between characters, set 1 or 2ms. Send 'u', you will be prompted to send the HEX file. Send the file. Done.

Connection diagram

| Arduino | STC1000   |
|---------|-----------|
| 9       | ICSPCLK   |
| 8       | ICSPDAT   |
| GND     | GND       |
| 5V      | VDD       |
| 3       | nMCLR     |


![alt text](https://raw.github.com/matsstaff/stc1000p/master/stc1000_ICSP.jpg "STC-1000 connection header")
Close up of my development STC-1000 that shows where the pins are located. Note that the pin header was added, and is not included.

The idea for the Arduino sketch came from [here](http://forum.arduino.cc/index.php?topic=92929.0), but was completely rewritten.

To modify the firmware, you will need a fresh installation of SDCC and GPUTILS. The source is pretty well commented. The MCU is pretty darn small and especially RAM is scarce. So you need to be very careful when programming. Avoid (especially nested) function calls, minimize global and static variables. Read the SDCC manual and PIC16F1828 datasheets. 

