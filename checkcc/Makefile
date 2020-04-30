all: checkc.bin

CCFLAGS=-std=c99 -Werror
CPP=gcc

checkc.bin: main.c *.h
	$(CPP) -O3 $(CCFLAGS) $< -lc -o $@

checkc-s.bin: main.c *.h
	$(CPP) -Os $(CCFLAGS) $< -lc -o $@

checkc-d.bin: main.c *.h
	$(CPP) -g -O0 $(CCFLAGS) $< -lc -o $@

clean:
	rm -r a.out* *.bin.* *.bin aout.c > /dev/null 2>&1