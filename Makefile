LIBS+= `pkg-config lua --libs` -pthread
CFLAGS+= -Wall -shared -fpic `pkg-config lua --cflags`

drone.so: drone.c
	gcc drone.c $(CFLAGS) $(LIBS) -o $@

