User Manual
===========

Note, after initial 

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

When running the programmed profile SP0 will be the initial setpoint, it will be held for dh0 hours. After that SP1 will
be used as setpoint for dh1 hours. The profile will stop running when a duration (dh) of 0 hours OR last step is 
reached (consider dh9 implicitly 0).
When the profile has ended, STC-1000+ will automatically switch to thermostat mode with the last reached setpoint. 
(So I guess you could also consider a dh value of 0 as infinite hours).

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
Every hour, 'dh' (and in next step is reached also 'St') is updated with new value. That means in case of a power outage, 
STC-1000+ will pick up (to within the hour) from where it left off.


