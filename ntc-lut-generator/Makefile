CC=gcc
OC=objcopy
ODIR=thermistor1.0

all:	lut clean

lut:	libcoeff.o librtot.o thermistor.h lut.c
	$(CC) lut.c -o lut libcoeff.o librtot.o -lm

clean:
	rm -f $(ODIR)/coeff.o $(ODIR)/rtot.o libcoeff.o librtot.o

$(ODIR)/coeff.o: $(ODIR)/coeff.c
	$(CC) -c $(ODIR)/coeff.c -o $(ODIR)/coeff.o -lm

$(ODIR)/rtot.o: $(ODIR)/rtot.c
	$(CC) -c $(ODIR)/rtot.c -o $(ODIR)/rtot.o -lm

libcoeff.o: $(ODIR)/coeff.o
	$(OC) -N main $(ODIR)/coeff.o libcoeff.o

librtot.o: $(ODIR)/rtot.o
	$(OC) -N main $(ODIR)/rtot.o librtot.o

