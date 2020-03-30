all: release debug small

CCFLAGS=-std=c++11 -nodefaultlibs -flto   -fno-rtti -fno-exceptions -fno-threadsafe-statics
CPP=g++

release: check.cc
	$(CPP) -O3 $(CCFLAGS) check.cc -lc -o checkcc

small: check.cc
	$(CPP) -Os $(CCFLAGS) check.cc -lc -o checkcc-s

debug: check.cc
	$(CPP) -g -O0 $(CCFLAGS) check.cc -lc -o checkcc-dbg
