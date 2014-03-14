CC=sdcc
CFLAGS_C=--use-non-free -mpic14 -p16f1828 --opt-code-size --no-pcode-opt --stack-size 16
CFLAGS_F=$(CFLAGS_C) -DFAHRENHEIT
DEPS = stc1000p.h
OBJ_C = page0_c.o page1_c.o eepromdata_c.o
OBJ_F = page0_f.o page1_f.o eepromdata_f.o

%_c.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS_C)

%_f.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS_F)

stc1000p_celsius: $(OBJ_C)
	$(CC) -o $@ $^ $(CFLAGS_C) $(LIBS)

stc1000p_fahrenheit: $(OBJ_F)
	$(CC) -o $@ $^ $(CFLAGS_F) $(LIBS)

all: stc1000p_celsius stc1000p_fahrenheit

.PHONY: clean

clean:
	rm -f *.o *~ core *.asm *.lst *.cod