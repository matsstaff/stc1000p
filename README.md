stc1000+
========

Firmware mod and Arduino based uploader for STC-1000 dual stage thermostat.

The STC-1000 is a dual stage (heating and cooling) thermostat that is pretty cheap to buy. I (and a low of fellow homebrewers) use them to control the fermentation temperature of beer.

The goal of this project is twofold
 * To create the means to reprogram the STC-1000 with a custom firmware
 * To create the custom firmware, suited for fermenting beer

The first goal is achieved by using an Arduino UNO with a sketch to act as a programmer, and the second by using the SDCC compiler and GPUTILS. 

A word of caution though. This project is in its infancy yet, and as the original firmware is protected, there is no way to restore the original functionality. And I assume no responsibilities.

To reprogram the STC-1000, make sure to **UNPLUG** (not just power off) STC-1000. Connect the necessary wires to the Arduino (see image),  download the sketch and suitible HEX file. Upload the sketch using Arduino IDE. Connect with a serial terminal emulator (115200 bps). I use CuteCom under GNU/Linux. It allows to set a delay between characters, set 1 or 2ms (if your terminal emulator dont support this, you could try to lower the bps considerably in the sketch). Send 'u', you will be prompted to send the HEX file. Send the file. Done.

Connection diagram

| Arduino | STC1000   | Notes |
|---------|-----------|-------|
| 9       | ICSPCLK   | Optionally connect via a ~1k resistor for protection | 
| 8       | ICSPDAT   | This really should be connected via a ~1k resistor for protection |
| GND     | GND       | |
| 5V      | VDD       | Optionally connect via a diode for protection |
| 3       | nMCLR     | |


![alt text](https://raw.github.com/matsstaff/stc1000p/master/stc1000_ICSP.jpg "STC-1000 connection header")
Close up of my development STC-1000 that shows where the pins are located. Note that the pin header was added, and is not included.

The idea for the Arduino sketch came from [here](http://forum.arduino.cc/index.php?topic=92929.0), but was completely rewritten.

Oh yeah, the internal piezo buzzer is connected to the ICSPDAT line, so during upload it will make a little noise. Kind of reminds me of the olden days with dialup connections :)

To modify the firmware, you will need a fresh installation of SDCC and GPUTILS. The source is pretty well commented. The MCU is pretty darn small and especially RAM is scarce. So you need to be very careful when programming. Avoid (especially nested) function calls, minimize global and static variables. Read the SDCC manual and PIC16F1828 datasheets. 

User Manual
===========

Note, after upload, the EEPROM is wiped. And the user needs to set values in the menu manually.

By default current temperature is displayed in C or F, depending on which firmware is used.
Pressing the 'S' button enters the menu. Pressing button 'up' and 'down' scrolls through the menu items. 
Button 'S' selects and 'power' button steps back or cancels current selection.

The menu is divided in two steps. When first pressing 'S', the following choices are presented:

|Menu item|Description|
|---|---|
|Pr0|Set parameters for profile 0|
|Pr1|Set parameters for profile 1|
|Pr2|Set parameters for profile 2|
|Pr3|Set parameters for profile 3|
|Pr4|Set parameters for profile 4|
|Pr5|Set parameters for profile 5|
|Set|Settings menu|

Selecting one of the profiles enters the the following submenu

Pr0-5 submenus have the following items

|Sub menu item|Description|Values|
|---|---|---|
SP0|Set setpoint 0|\-40.0 to 140C or \-40.0 to 250F
dh0|Set duration 0|0 to 999 hours
...|Set SP/dh x   |...
dh8|Set duration 8|0 to 999 hours
SP9|Set setpoint 9|\-40.0 to 140C or \-40.0 to 250F

When running the programmed profile, SP0 will be the initial setpoint, it will be held for dh0 hours. After that SP1 will be used as setpoint for dh1 hours. The profile will stop running when a duration (dh) of 0 hours OR last step is 
reached (consider 'dh9' implicitly 0).
When the profile has ended, STC-1000+ will automatically switch to thermostat mode with the last reached setpoint. 
(So I guess you could also consider a 'dh' value of 0 as infinite hours).

Settings menu has the following items

|Sub menu item|Description|Values|
|---|---|---|
hy|Set hysteresis|0.0 to 2.5C or 0.0 to 5.0F
tc|Set temperature correction|\-2.5 to 2.5C or \-5.0 to 5.0F
SP|Set setpoint|\-40 to 140C or \-40 to 250F
St|Set current profile step|0 to 8
dh|Set current profile duration|0 to 999 hours
cd|Set cooling delay|0 to 60 minutes
rn|Set run mode|'Pr0' to 'Pr5' and 'th'

Hysteresis, is the allowable temperature range around the setpoint in which no cooling or heating will occur.
Temperature correction, will be added to the read temperature, allows the user to calibrate temperature reading.
Setpoint, well... The desired temperature to keep.
Current profile step, allows 'jumping' in the profile.
Current profile duration in the step, allows 'jumping' in the profile. Step and duration are updated automatically when 
running the profile, but can also be set manually at any time.
Run mode, selecting 'Pr0' to 'Pr5' will start the corresponding profile running from step 0, duration 0. Selecting 'th' 
will switch to thermostat mode (i.e. stop any running profile, setpoint will not change from this).

The way that STC-1000+ is implemented, the profile will automatically set 'SP' when a new step is reached in the profile. 
That means when running a profile, 'SP' is NOT preserved.
Every hour, 'dh' (and if next step is reached, also 'St') is updated with new value(s). That means in case of a power outage, STC-1000+ will pick up (to within the hour) from where it left off.


