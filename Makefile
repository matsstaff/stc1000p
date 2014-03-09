CC=sdcc
#CFLAGS=--use-non-free -mpic14 -p16f1828 --opt-code-size --no-pcode-opt
CFLAGS=--use-non-free -mpic14 -p16f1828 --opt-code-size --no-pcode-opt -DFAHRENHEIT
DEPS = 
OBJ = page0.o page1.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

stc1000p: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

all: stc1000p

.PHONY: clean

clean:
	rm -f *.o *~ core *.asm *.lst *.cod