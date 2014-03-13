stc1000+
========

Firmware mod and Arduino based uploader for STC-1000 dual stage thermostat.

The STC-1000 is a dual stage (heating and cooling) thermostat that is pretty cheap to buy. I (and a low of fellow homebrewers) use them to control the fermentation temperature of beer.

The goal of this project is twofold
 * To create the means to reprogram the STC-1000 with a custom firmware
 * To create the custom firmware, suited for fermenting beer

The first goal is achieved by using an Arduino UNO with a sketch to act as a programmer, and the second by using the SDCC compiler and GPUTILS. 

A word of caution though. This project is in its infancy yet, and as the original firmware is protected, there is no way to restore the original functionality. And I assume no responsibilities.

Uploading new firmware
======================

To reprogram the STC-1000, download the sketch (picprog.ino, this is the only file you need unless you're a developer) and open it with the Arduino IDE.
Upload the sketch using Arduino IDE to the Arduino. 
Make sure to **UNPLUG** (not just power off) STC-1000. Also, be sure to **have the sensor connected**, otherwise the STC-1000 will set off the alarm when you connect the wires.
Connect the necessary wires to the Arduino (see images and table below). 

Connection diagram

| Arduino | STC1000   | Notes |
|---------|-----------|-------|
| 9       | ICSPCLK   | Optionally connect via a ~1k resistor for protection | 
| 8       | ICSPDAT   | Optionally connect via a ~1k resistor for protection |
| GND     | GND       | |
| 5V      | VDD       | Optionally connect via a diode for protection |
| 3       | nMCLR     | |

The resistors and diode are optional and really should not be needed, but would be nice to include if you would build a shield or a more professional programmer. Note, that you should upload the sketch BEFORE making the connections, as I have no idea what is running on the Arduino previously. The resistors will protect your Arduino and STC-1000 in the case this happens.

The sticker needs to be cut or removed.
![alt text](http://i62.tinypic.com/2rf3ts5.jpg "Cut the sticker")

Lift the tabs with a small screwdriver or your fingernails.
![alt text](http://i60.tinypic.com/b9j7gm.jpg "Lift tab and pull board out")

Locate the programming header. This is where the connections needs to be made to the Arduino. It is best to solder a pin header for easy access, but soldering wires directly will work. If you go the pinheader route, I have found it easies to use a desoldering vacuum pump, to suck out the solder. Then you can fitting the header is quite easy. If you can't solder I guess you could use hot glue or tape or something to make a temporary connection.

![alt text](http://i61.tinypic.com/213j1v7.jpg "Locate programming header")

It may be easier to access the header from the bottom of the board.
![alt text](http://i60.tinypic.com/k970o7.jpg "Might be easier to access from the bottom")

This is my development setup for reference. Sorry for the crappy picture quality.
![alt text](http://i62.tinypic.com/11ue2rd.jpg "Example setup")

Close up of my development STC-1000 that shows where the pins are located. Note that the pin header was added, and is not included with the STC-1000.

Connect with a the Arduino IDE serial monitor (Ctrl+Shift+M), select '115200 baud' and 'No line ending'. 
Place cursor in the top field of the serial monitor, type 'd' (that is a single d, not the apostrophes) and send.
If all is connected correctly you should be greeted with 'STC-1000 detected.', if not check wiring and settings. If it passed, you can send 'a' to upload Celsius version or 'f' for Fahrenheit version. Uploading takes approx 20 seconds and during that time the STC-1000 will make some noise, due to how the hardware was designed. That is perfectly normal.
After flashing new firmware, EEPROM settings are wiped and you'll need to set values using the buttons (see User Manual).

The idea for the Arduino sketch came from [here](http://forum.arduino.cc/index.php?topic=92929.0), but was completely rewritten.

To modify the firmware yourself, you will need a fresh installation of SDCC and GPUTILS. The source is pretty well commented. The MCU is pretty darn small and especially RAM is scarce. So you need to be very careful when programming. Avoid (especially nested) function calls, minimize global and static variables. Read the SDCC manual and PIC16F1828 datasheets. 

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
hd|Set heating delay|0 to 4 minutes
rn|Set run mode|'Pr0' to 'Pr5' and 'th'

Hysteresis, is the allowable temperature range around the setpoint in which no cooling or heating will occur.
Temperature correction, will be added to the read temperature, allows the user to calibrate temperature reading.
Setpoint, well... The desired temperature to keep.
Current profile step, allows 'jumping' in the profile.
Current profile duration in the step, allows 'jumping' in the profile. Step and duration are updated automatically when 
running the profile, but can also be set manually at any time. (Note at the time of writing, updating current step will not take effect until next step/duration calculation occurs, which might be at most one hour).
Run mode, selecting 'Pr0' to 'Pr5' will start the corresponding profile running from step 0, duration 0. Selecting 'th' 
will switch to thermostat mode (i.e. stop any running profile, setpoint will not change from this).

The way that STC-1000+ is implemented, the profile will automatically set 'SP' when a new step is reached in the profile. 
That means when running a profile, 'SP' is NOT preserved.
Every hour, 'dh' (and if next step is reached, also 'St') is updated with new value(s). That means in case of a power outage, STC-1000+ will pick up (to within the hour) from where it left off.


