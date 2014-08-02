#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "thermistor.h"

#define C_TO_F(t)	((t)*1.8 + 32.0)

/**
 * Convert A/D value to probe resistance value
 * @param ad_value The A/D value (0-1024)
 * @return The probe resistance (in Ohm) 
 */
double ad_to_r(int ad_value){
	return (((R0 * AD_MAX) / ad_value) - R0);
}

int main(int argc, char *argv[]){
	double r;
	int i;
	int lut_c[32];
	int lut_f[32];
	const int ad_lookup_c[32] = { 0, -470, -357, -273, -206, -152, -104, -61, -22, 16, 51, 85, 119, 151, 185, 217, 250, 283, 318, 354, 391, 431, 473, 519, 568, 624, 688, 763, 857, 975, 1150, 1400 };
	const int ad_lookup_f[32] = { 0, -526, -322, -171, -51, 47, 132, 210, 280, 348, 412, 473, 534, 592, 653, 711, 770, 829, 892, 958, 1024, 1096, 1171, 1253, 1343, 1443, 1558, 1693, 1862, 2075, 2390, 2840 };
	polynom *erg;

	if(argc!=2){
		printf("Usage: %s filename\n", argv[0]);
		exit(-1);
	}

	readtable(argv[1]);
	orthonormal(basis);
	erg = approx();
	testresult(erg);

	for(i=0; i<4; i++){
		a[i] = (*erg)[i];
	}

	lut_c[0]=0;
	lut_f[0]=0;

	for(i=1; i<32; i++){
		r = rtot(ad_to_r(i*32));
		lut_c[i] = (int)round(r * 10.0); 
		lut_f[i] = (int)round(C_TO_F(r) * 10.0); 
	}

	printf("//Celsius\n");
	printf("const int ad_lookup[32] = { 0");
	for(i=1; i<32; i++){
		printf(", %d", lut_c[i]);
	}
	printf(" };\n");
	
	printf("//Fahrenheit\n");
	printf("const int ad_lookup[32] = { 0");
	for(i=1; i<32; i++){
		printf(", %d", lut_f[i]);
	}
	printf(" };\n");

}

