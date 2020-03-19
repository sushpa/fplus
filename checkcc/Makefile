all: release debug

CCFLAGS=-std=c++11 -nodefaultlibs -fno-rtti -fno-exceptions -fno-threadsafe-statics -lc
CPP=g++

release: check.cc
	$(CPP) -O3 $(CCFLAGS) check.cc -o checkcc

debug: check.cc
	$(CPP) -g -O0 $(CCFLAGS) check.cc -o checkcc-dbg

