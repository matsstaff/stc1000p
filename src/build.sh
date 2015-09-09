#/bin/bash

# STC1000+, improved firmware and Arduino based firmware uploader for the STC-1000 dual stage thermostat.
#
# Copyright 2014 Mats Staffansson
#
# This file is part of STC1000+.
#
# STC1000+ is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# STC1000+ is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with STC1000+.  If not, see <http://www.gnu.org/licenses/>.

# This is a simple script to make building STC-1000+ releases easier.

# Extract version info from stc1000p.h
v=`cat stc1000p.h | grep STC1000P_VERSION`
e=`cat stc1000p.h | grep STC1000P_EEPROM_VERSION`

# Create stc1000p.js
cat picprog.template | sed "s/^#define STC1000P_VERSION.*/$v/" | sed "s/^#define STC1000P_EEPROM_VERSION.*/$e/" | sed "s/'/\\\\\\\'/g" > stc1000p.js.tmp
echo "var stc1000p={};" > ../profile/stc1000p.js
echo "stc1000p[\"picprog\"]='' +" >> ../profile/stc1000p.js
while IFS= read r; do
	echo "'$r\n' +"; 
done < stc1000p.js.tmp >> ../profile/stc1000p.js
echo "'';" >> ../profile/stc1000p.js
rm -f stc1000p.js.tmp

function build_stc1000p_version {
	
	if [[ $1 != "" ]]; then 
		version=_$1
	else
		version=$1
	fi
	
	# Build HEX
	echo "";
	echo "Building stc1000p$version"
	make stc1000p$version

	echo "";
	echo "Create picprog$version.ino";

	# Create sketch and insert version info
	cat picprog.template | sed "s/^#define STC1000P_VERSION.*/$v/" | sed "s/^#define STC1000P_EEPROM_VERSION.*/$e/" > ../picprog$version.ino

	# Convert firmware HEX data to embed in sketch
	echo "const char hex_celsius[] PROGMEM = {" > stc1000p_hex.tmp; 
	for l in `cat build/stc1000p_celsius$version.hex | sed 's/^://' | sed 's/\(..\)/0\x\1\,/g'`; do 
		echo "   $l" | sed 's/0x00,0x00,0x00,0x01,0xFF,/0x00,0x00,0x00,0x01,0xFF/' >> stc1000p_hex.tmp; 
	done; 
	echo "};" >> stc1000p_hex.tmp

	echo "const char hex_fahrenheit[] PROGMEM = {" >> stc1000p_hex.tmp; 
	for l in `cat build/stc1000p_fahrenheit$version.hex | sed 's/^://' | sed 's/\(..\)/0\x\1\,/g'`; do 
		echo "   $l" | sed 's/0x00,0x00,0x00,0x01,0xFF,/0x00,0x00,0x00,0x01,0xFF/' >> stc1000p_hex.tmp; 
	done; 
	echo "};" >> stc1000p_hex.tmp

	cat stc1000p_hex.tmp | sed "s/'/\\\\\\\'/g" > stc1000p_hex.js.tmp
	echo "stc1000p[\"stc1000p$version\"]='' +" >> ../profile/stc1000p.js
	while IFS= read r; do
		echo "'$r\n' +"; 
	done < stc1000p_hex.js.tmp >> ../profile/stc1000p.js
	echo "'';" >> ../profile/stc1000p.js
	rm -f stc1000p_hex.js.tmp

	echo "const char hex_eeprom_celsius[] PROGMEM = {" >> stc1000p_hex.tmp; 
	for l in `cat build/eedata_celsius$version.hex | sed 's/^://' | sed 's/\(..\)/0\x\1\,/g'`; do 
		echo "   $l" | sed 's/0x00,0x00,0x00,0x01,0xFF,/0x00,0x00,0x00,0x01,0xFF/' >> stc1000p_hex.tmp; 
	done; 
	echo "};" >> stc1000p_hex.tmp

	echo "const char hex_eeprom_fahrenheit[] PROGMEM = {" >> stc1000p_hex.tmp; 
	for l in `cat build/eedata_fahrenheit$version.hex | sed 's/^://' | sed 's/\(..\)/0\x\1\,/g'`; do 
		echo "   $l" | sed 's/0x00,0x00,0x00,0x01,0xFF,/0x00,0x00,0x00,0x01,0xFF/' >> stc1000p_hex.tmp; 
	done; 
	echo "};" >> stc1000p_hex.tmp

	cat stc1000p_hex.tmp >> ../picprog$version.ino
	rm -f stc1000p_hex.tmp

	# Print size approximation (from .asm files)
	echo "";
	echo "Size";
	let s=0;
	for a in `cat build/page0_celsius$version.asm build/page1_celsius$version.asm | grep instructions | sed 's/^.*=  //' | sed 's/ instructions.*//'`;
	do
		echo $a;
		s=$(($s+$a));
	done;
	echo "total $s";

}

build_stc1000p_version
build_stc1000p_version "probe2";
build_stc1000p_version "com";
build_stc1000p_version "fo433";
build_stc1000p_version "minute";
build_stc1000p_version "minute_probe2";
build_stc1000p_version "minute_com";
build_stc1000p_version "minute_fo433";
build_stc1000p_version "ovbsc";

make clean

