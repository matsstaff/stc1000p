#ifndef _THERMISTOR_H_
#define _THERMISTOR_H_

#define R0_DEFAULT	(10000.0)
#define AD_MAX_DEFAULT	(1024)
#define AD_STEP_DEFAULT	(32)

#define M		4

/** Type definition for a polynom. */
typedef double polynom[M];

extern double a[M];
extern polynom basis[M];

/* Evaluate p(x) */
double value(polynom p, double x);
/* Evaluate [p,q] */
double skalarpoly(polynom p, polynom q);
/* Evaluate p *= fact */
void mult(polynom p, double fact);
/* Evaluate p += fact*q */
void linear(polynom p, polynom q, double fact);
/* Build orthonormal base p from p */
void orthonormal(polynom p[]);
/* Evaluate [p, pf] */
double skalar(polynom p);
/* Evaluate approximating polynom. */
polynom *approx(void);
/* Reads all temperature- resistance pairs from an T-R table file. */
void readtable(char *filename);
/* Tests the approximation polynom with all t-r pairs. */
void testresult(polynom *erg);
/* Exits with error message in case of errors. */
void errexit(char *format, ...);

/* Evaluates p(x) for a polynom p. */
double poly(double x, int degree, double p[]);
/* Conversion from resistance to temperature. */
double rtot(double r);

#endif // _THERMISTOR_H_

