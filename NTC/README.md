This directory contains a small project to make updates to the NTC lookup table a lot easier and robust.
It makes use of the 'NTC thermistor library' http://thermistor.sourceforge.net/

The library needs to be downloaded and unzipped into a folder ('thermistor.1.0').
Direct linkt for download:
http://sourceforge.net/projects/thermistor/files/C%20thermistor%20library/1.0/thermistor.1.0.zip/download

The make file is targeted for GCC. Just run make, and if all is well, an excutable ('lut') will be created.

Run lut with a single argument that is a file containing resistance-temperature values, separated by a single tab.
For example './lut vishay.txt'

This will make use of the NTC thermistor library to calculate Steinhart-Hart coefficients based off the supplied data,
and then use that to calculate the expected temperature at the A/D lookup points.
It will output a lot of stuff, but the last few lines will be the actual code lines to go into STC-1000+ source (in page0.c).

The current model used for STC-1000+ is the data in vishay.txt.
As far as I know, the sensor shipped with the STC-1000 is a 10k NTC thermistor with a Beta(25-85) of 3435. Presumably 1%.