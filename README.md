STC\-1000+
========

Improved firmware for fermentation and Arduino based uploader for the STC-1000 dual stage thermostat.

![STC-1000](http://img.diytrade.com/cdimg/1066822/11467124/0/1261107339/temperature_controllers_STC-1000.jpg)

The STC-1000 is a dual stage (heating and cooling) thermostat that is pretty cheap to buy. I (and a low of fellow homebrewers) use them to control the fermentation temperature of beer.

The goal of this project is twofold
 * To create the means to reprogram the STC-1000 with a custom firmware
 * To create a custom firmware, suited for fermenting beer

The first goal is achieved by using an Arduino UNO with a sketch to act as a programmer, and the second by using the SDCC compiler and GPUTILS. 

If you are new to GitHub, use the 'Download ZIP' button ->
and start by reading usermanual.pdf

Features
--------

* Both Fahrenheit and Celsius versions
* Up to 6 profiles with up to 10 setpoints.
* Each setpoint can be held for 1-999 hours (i.e. up to ~41 days).
* Somewhat intuitive menus for configuring
* Separate delay settings for cooling and heating
* Configurable hysteresis (allowable temp swing) from 0.0 to 2.5°C or 0.0 to 5.0°F
* Approximative ramping
* Button acceleration, for frustrationless programming by buttons

Cheers!


Updates
=======

2014-04-04 Release v1.00
