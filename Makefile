all: checkc.bin

CCFLAGS=-std=c99 -Werror -Imodules
CPP=gcc

modules/TokenKindDefs.h: modules/makeTokens.sh
	cd modules && ./makeTokens.sh

checkc.bin: programs/main.c modules/*.h
	$(CPP) -O0 $(CCFLAGS) $< -lc -o $@

checkc-s.bin: programs/main.c modules/*.h
	$(CPP) -Os $(CCFLAGS) $< -lc -o $@

checkc-d.bin: programs/main.c modules/*.h
	$(CPP) -g -O0 $(CCFLAGS) $< -lc -o $@

checkc-cov.bin: programs/main.c modules/*.h
	$(CPP) -g -fprofile-arcs -ftest-coverage -O0 $(CCFLAGS) $< -lc -o $@

clean:
	@rm -r a.out* *.bin.* *.bin aout.c > /dev/null 2>&1; true
