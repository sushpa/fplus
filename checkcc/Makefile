all: checkc.bin

CCFLAGS=-std=c99 -Werror
CPP=gcc

TokenKindDefs.h: makeTokens.sh
	./makeTokens.sh

checkc.bin: main.c chstd.h cycle.h errors.h gen.h genc.h gencpp.h genlua.h hash.h resolve.h token.h tokenKind.h typecheck.h parse.h TokenKindDefs.h types.h stats.h
	$(CPP) -O0 $(CCFLAGS) $< -lc -o $@

checkc-s.bin: main.c chstd.h cycle.h errors.h gen.h genc.h gencpp.h genlua.h hash.h resolve.h token.h tokenKind.h typecheck.h parse.h TokenKindDefs.h types.h stats.h
	$(CPP) -Os $(CCFLAGS) $< -lc -o $@

checkc-d.bin: main.c chstd.h cycle.h errors.h gen.h genc.h gencpp.h genlua.h hash.h resolve.h token.h tokenKind.h typecheck.h parse.h TokenKindDefs.h types.h stats.h
	$(CPP) -g -O0 $(CCFLAGS) $< -lc -o $@

clean:
	@rm -r a.out* *.bin.* *.bin aout.c > /dev/null 2>&1; true