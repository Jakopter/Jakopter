ARCH := $(shell uname -m)

ifeq ($(ARCH), x86_64)
LEAP_LIBRARY := lib/x64/libLeap.so -Wl,-rpath,lib/x64
else
LEAP_LIBRARY := lib/x86/libLeap.so -Wl,-rpath,lib/x86
endif

Leap: Leap.cpp
	$(CXX) -Wall -g -I../include Leap.cpp -o Leap $(LEAP_LIBRARY)

clean:
	rm -rf Leap
