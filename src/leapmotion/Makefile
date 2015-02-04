ARCH := $(shell uname -m)
LIBS+= `pkg-config lua --libs` `pkg-config libavcodec --libs` -pthread
ifeq ($(ARCH), x86_64)
	LEAP_LIBRARY := lib/x64/libLeap.so -Wl,-rpath,lib/x64
else
	LEAP_LIBRARY := lib/x86/libLeap.so -Wl,-rpath,lib/x86
endif
DRONE := lib/x64/libjakopter.so
FLAGS := -Wall -g -fdiagnostics-color=auto

Leap: Leap.cpp
#	$(CXX) -Wall -g -I../../include Leap.cpp -o Leap $(LEAP_LIBRARY)
#	g++ -Wall -g -I../../include Leap.cpp -o Leap lib/x64/libjakopter.so -Wl,-rpath,lib/x64 lib/x64/libLeap.so -Wl,-rpath,lib/x64 $(LIBS)
#	g++ $(FLAGS) -I../../include Leap.cpp -o Leap -L./lib/x64 -lLeap -Wl,-rpath=./lib/x64 $(LIBS) -L./lib/x64 -ljakopter -Wl,-rpath=./lib/x64
	clang++ -L./lib/x64 -I/home/miel/repos/Jakopter/src/leapmotion/include Leap.cpp -o Leap -lLeap $(LIBS) -ljakopter

clean:
	rm -rf Leap
