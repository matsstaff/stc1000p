CC=sdcc
CFLAGS=--use-non-free -mpic14 -p16f1828 --opt-code-size --no-pcode-opt --stack-size 8
OUTDIR=build
DEPS = stc1000p.h

$(OUTDIR)/page0_%.o: page0.c stc1000p.h
	$(CC) -c -o $@ $< $(CFLAGS) $(VFLAGS)

$(OUTDIR)/page1_%.o: page1.c stc1000p.h
	$(CC) -c -o $@ $< $(CFLAGS) $(VFLAGS)

stc1000p_%: $(OUTDIR)/page0_%.o $(OUTDIR)/page1_%.o
	$(CC) -o $(OUTDIR)/$@ $^ $(CFLAGS) $(VFLAGS)

$(OUTDIR)/eedata_%.o: eepromdata.c stc1000p.h
	$(CC) -c -o $@ $< $(CFLAGS) $(VFLAGS)

eedata_%: $(OUTDIR)/eedata_%.o
	$(CC) -o $(OUTDIR)/$@ $^ $(CFLAGS) $(VFLAGS)

stc1000p_celsius: VFLAGS := 
stc1000p_fahrenheit: VFLAGS := -DFAHRENHEIT
eedata_celsius: VFLAGS := 
eedata_fahrenheit: VFLAGS := -DFAHRENHEIT
stc1000p: stc1000p_celsius stc1000p_fahrenheit eedata_celsius eedata_fahrenheit

stc1000p_celsius_probe2: VFLAGS := -DPB2
stc1000p_fahrenheit_probe2: VFLAGS := -DFAHRENHEIT -DPB2
eedata_celsius_probe2: VFLAGS := -DPB2
eedata_fahrenheit_probe2: VFLAGS := -DFAHRENHEIT -DPB2
stc1000p_probe2: stc1000p_celsius_probe2 stc1000p_fahrenheit_probe2 eedata_celsius_probe2 eedata_fahrenheit_probe2

stc1000p_celsius_com: VFLAGS := -DCOM
stc1000p_fahrenheit_com: VFLAGS := -DFAHRENHEIT -DCOM
eedata_celsius_com: VFLAGS := -DCOM
eedata_fahrenheit_com: VFLAGS := -DFAHRENHEIT -DCOM
stc1000p_com: stc1000p_celsius_com stc1000p_fahrenheit_com eedata_celsius_com eedata_fahrenheit_com

stc1000p_celsius_fo433: VFLAGS := -DFO433
stc1000p_fahrenheit_fo433: VFLAGS := -DFAHRENHEIT -DFO433
eedata_celsius_fo433: VFLAGS := -DFO433
eedata_fahrenheit_fo433: VFLAGS := -DFAHRENHEIT -DFO433
stc1000p_fo433: stc1000p_celsius_fo433 stc1000p_fahrenheit_fo433 eedata_celsius_fo433 eedata_fahrenheit_fo433

stc1000p_celsius_minute: VFLAGS := -DMINUTE
stc1000p_fahrenheit_minute: VFLAGS := -DFAHRENHEIT -DMINUTE
eedata_celsius_minute: VFLAGS := -DMINUTE
eedata_fahrenheit_minute: VFLAGS := -DFAHRENHEIT -DMINUTE
stc1000p_minute: stc1000p_celsius_minute stc1000p_fahrenheit_minute eedata_celsius_minute eedata_fahrenheit_minute

stc1000p_celsius_minute_probe2: VFLAGS := -DMINUTE -DPB2
stc1000p_fahrenheit_minute_probe2: VFLAGS := -DFAHRENHEIT -DMINUTE -DPB2
eedata_celsius_minute_probe2: VFLAGS := -DMINUTE -DPB2
eedata_fahrenheit_minute_probe2: VFLAGS := -DFAHRENHEIT -DMINUTE -DPB2
stc1000p_minute_probe2: stc1000p_celsius_minute_probe2 stc1000p_fahrenheit_minute_probe2 eedata_celsius_minute_probe2 eedata_fahrenheit_minute_probe2

stc1000p_celsius_minute_com: VFLAGS := -DMINUTE -DCOM
stc1000p_fahrenheit_minute_com: VFLAGS := -DFAHRENHEIT -DMINUTE -DCOM
eedata_celsius_minute_com: VFLAGS := -DMINUTE -DCOM
eedata_fahrenheit_minute_com: VFLAGS := -DFAHRENHEIT -DMINUTE -DCOM
stc1000p_minute_com: stc1000p_celsius_minute_com stc1000p_fahrenheit_minute_com eedata_celsius_minute_com eedata_fahrenheit_minute_com

stc1000p_celsius_minute_fo433: VFLAGS := -DMINUTE -DFO433
stc1000p_fahrenheit_minute_fo433: VFLAGS := -DFAHRENHEIT -DMINUTE -DFO433
eedata_celsius_minute_fo433: VFLAGS := -DMINUTE -DFO433
eedata_fahrenheit_minute_fo433: VFLAGS := -DFAHRENHEIT -DMINUTE -DFO433
stc1000p_minute_fo433: stc1000p_celsius_minute_fo433 stc1000p_fahrenheit_minute_fo433 eedata_celsius_minute_fo433 eedata_fahrenheit_minute_fo433

stc1000p_celsius_ovbsc: VFLAGS := -DOVBSC
stc1000p_fahrenheit_ovbsc: VFLAGS := -DFAHRENHEIT -DOVBSC
eedata_celsius_ovbsc: VFLAGS := -DOVBSC
eedata_fahrenheit_ovbsc: VFLAGS := -DFAHRENHEIT -DOVBSC
stc1000p_ovbsc: stc1000p_celsius_ovbsc stc1000p_fahrenheit_ovbsc eedata_celsius_ovbsc eedata_fahrenheit_ovbsc

all: stc1000p stc1000p_probe2 stc1000p_com stc1000p_fo433 stc1000p_minute stc1000p_minute_probe2 stc1000p_minute_com stc1000p_minute_fo433 stc1000p_ovbsc

.PHONY: clean stc1000p stc1000p_probe2 stc1000p_com stc1000p_fo433 stc1000p_minute stc1000p_minute_probe2 stc1000p_minute_com stc1000p_minute_fo433 stc1000p_ovbsc

clean:
	rm -f $(OUTDIR)/*.o *~ core $(OUTDIR)/*.asm $(OUTDIR)/*.lst $(OUTDIR)/*.cod
