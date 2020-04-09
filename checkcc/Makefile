all: checkc.bin

CCFLAGS=-std=c99
CPP=gcc

checkc.bin: main.c chstd.h cycle.h
	$(CPP) -O3 $(CCFLAGS) $< -lc -o $@

checkc-s.bin: main.c chstd.h cycle.h
	$(CPP) -Os $(CCFLAGS) $< -lc -o $@

checkc-d.bin: main.c chstd.h cycle.h
	$(CPP) -g -O0 $(CCFLAGS) $< -lc -o $@

clean:
	rm -r a.out* *.bin.* *.bin aout.c