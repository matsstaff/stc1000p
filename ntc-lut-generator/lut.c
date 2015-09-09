#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "thermistor.h"

#define C_TO_F(t)	((t)*1.8 + 32.0)

double R0=R0_DEFAULT;
int AD_MAX=AD_MAX_DEFAULT;
int AD_STEP=AD_STEP_DEFAULT;

#define LUT_SIZE	(AD_MAX / AD_STEP)

/**
 * Convert A/D value to probe resistance value
 * @param ad_value The A/D value (0-1024)
 * @return The probe resistance (in Ohm) 
 */
double ad_to_r(int ad_value){
	return (((R0 * AD_MAX) / ad_value) - R0);
}

void usage(char *cmd){
	printf("Calculate A/D lookup table for NTC thermistor / resistor voltage divider network\n");
	printf("using temperature-resstance data points from a file for the thermistor as input.\n");
	printf("Usage: %s [-r R0 (default=10000.0 ohm)] [-m AD_MAX (default=1024)] [-s AD_STEP (default=32)] filename\n", cmd);	
}

int main(int argc, char *argv[]){
	double r;
	int i;
	int *lut_c;
	int *lut_f;
	polynom *erg;

	/* getopt */
	int c;

	opterr = 0;

	while((c=getopt(argc, argv, "r:m:s:")) != -1) {
		switch(c) {
			case 'r':
				R0=atof(optarg);
				break;
			case 'm':
				AD_MAX=atoi(optarg);
				break;
			case 's':
				AD_STEP=atoi(optarg);
				break;
			case '?':
				if (optopt == 'r' || optopt == 'm' || optopt == 's'){
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				} else if (isprint (optopt)) {
					fprintf(stderr, "Unknown option '-%c'.\n", optopt);
				} else {
					fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
				}
				/* fall through */
			default:
				usage(argv[0]);
				return 1;
		      }

	}

	if(optind != argc-1 || R0 <=0 || AD_MAX <=0 || AD_STEP <= 0){
		usage(argv[0]);
		return 1;
	}

	printf("Using: R0 = %lf, AD_MAX = %d, AD_STEP = %d\n", R0, AD_MAX, AD_STEP);

	readtable(argv[optind]);
	orthonormal(basis);
	erg = approx();
	testresult(erg);

	for(i=0; i<4; i++){
		a[i] = (*erg)[i];
	}

	lut_c = malloc(sizeof(int) * LUT_SIZE);
	lut_f = malloc(sizeof(int) * LUT_SIZE);

	lut_c[0]=0;
	lut_f[0]=0;

	for(i=1; i<LUT_SIZE; i++){
		r = rtot(ad_to_r(i*AD_STEP));
		lut_c[i] = (int)round(r * 10.0); 
		lut_f[i] = (int)round(C_TO_F(r) * 10.0); 
	}

	printf("//Celsius\n");
	printf("const int ad_lookup[] = { 0");
	for(i=1; i<LUT_SIZE; i++){
		printf(", %d", lut_c[i]);
	}
	printf(" };\n");
	
	printf("//Fahrenheit\n");
	printf("const int ad_lookup[] = { 0");
	for(i=1; i<LUT_SIZE; i++){
		printf(", %d", lut_f[i]);
	}
	printf(" };\n");

}

