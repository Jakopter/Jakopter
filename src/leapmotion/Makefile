ARCH := $(shell uname -m)

ifeq ($(ARCH), x86_64)
LEAP_LIBRARY := x64/libLeap.so -Wl,-rpath,x64
else
LEAP_LIBRARY := x86/libLeap.so -Wl,-rpath,x86
endif

Leap: Leap.cpp
	$(CXX) -Wall -g -I../include Leap.cpp -o Leap $(LEAP_LIBRARY)

clean:
	rm -rf Leap
