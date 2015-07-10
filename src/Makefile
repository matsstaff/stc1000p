CC=sdcc
CFLAGS=--use-non-free -mpic14 -p16f1828 --opt-code-size --no-pcode-opt --stack-size 8
OUTDIR=build
DEPS = stc1000p.h
OBJ_C = $(OUTDIR)/page0_c.o $(OUTDIR)/page1_c.o
OBJ_F = $(OUTDIR)/page0_f.o $(OUTDIR)/page1_f.o
OBJ_C_COM = $(OUTDIR)/page0_c_com.o $(OUTDIR)/page1_c_com.o
OBJ_F_COM = $(OUTDIR)/page0_f_com.o $(OUTDIR)/page1_f_com.o
OBJ_C_FO433 = $(OUTDIR)/page0_c_fo433.o $(OUTDIR)/page1_c_fo433.o
OBJ_F_FO433 = $(OUTDIR)/page0_f_fo433.o $(OUTDIR)/page1_f_fo433.o
OBJ_EEPROM_C = $(OUTDIR)/eepromdata_c.o
OBJ_EEPROM_F = $(OUTDIR)/eepromdata_f.o

$(OUTDIR)/%_c.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -DPB2

$(OUTDIR)/%_f.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -DFAHRENHEIT -DPB2

$(OUTDIR)/%_c_com.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -DCOM

$(OUTDIR)/%_f_com.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -DFAHRENHEIT -DCOM

$(OUTDIR)/%_c_fo433.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -DFO433

$(OUTDIR)/%_f_fo433.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -DFAHRENHEIT -DFO433

stc1000p_celsius: $(OBJ_C)
	$(CC) -o $(OUTDIR)/$@ $^ $(CFLAGS) $(LIBS)

stc1000p_fahrenheit: $(OBJ_F)
	$(CC) -o $(OUTDIR)/$@ $^ $(CFLAGS) -DFAHRENHEIT $(LIBS)

stc1000p_celsius_com: $(OBJ_C_COM)
	$(CC) -o $(OUTDIR)/$@ $^ $(CFLAGS) -DCOM $(LIBS)

stc1000p_fahrenheit_com: $(OBJ_F_COM)
	$(CC) -o $(OUTDIR)/$@ $^ $(CFLAGS) -DFAHRENHEIT -DCOM $(LIBS)

stc1000p_celsius_fo433: $(OBJ_C_FO433)
	$(CC) -o $(OUTDIR)/$@ $^ $(CFLAGS) -DFO433 $(LIBS)

stc1000p_fahrenheit_fo433: $(OBJ_F_FO433)
	$(CC) -o $(OUTDIR)/$@ $^ $(CFLAGS) -DFAHRENHEIT -DFO433 $(LIBS)

eedata_celsius: $(OBJ_EEPROM_C)
	$(CC) -o $(OUTDIR)/$@ $^ $(CFLAGS) $(LIBS)

eedata_fahrenheit: $(OBJ_EEPROM_F)
	$(CC) -o $(OUTDIR)/$@ $^ $(CFLAGS) -DFAHRENHEIT $(LIBS)

all: stc1000p_celsius stc1000p_fahrenheit stc1000p_celsius_com stc1000p_fahrenheit_com stc1000p_celsius_fo433 stc1000p_fahrenheit_fo433 eedata_celsius eedata_fahrenheit

.PHONY: clean

clean:
	rm -f $(OUTDIR)/*.o *~ core $(OUTDIR)/*.asm $(OUTDIR)/*.lst $(OUTDIR)/*.cod
