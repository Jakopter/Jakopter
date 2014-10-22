LIBS+= `pkg-config lua --libs` -pthread
LDFLAGS+= -shared
CFLAGS+= -c -std=gnu99 -Wall -fpic `pkg-config lua --cflags`

drone.so: drone.o navdata.o video.o common.h
	gcc $(LDFLAGS) $^ $(LIBS) -o $@
	
%.o: %.c
	gcc $(CFLAGS) $^ -o $@

clean: 
	rm -rf *.so *.o
