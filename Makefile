LIBS+= `pkg-config lua --libs` -pthread
CFLAGS+= -Wall -shared -fpic `pkg-config lua --cflags`

drone.so: drone.c navdata.c
	gcc -std=gnu99 navdata.c $(CFLAGS) $(LIBS) -o navdata.o -c
	gcc -std=gnu99 drone.c navdata.o $(CFLAGS) $(LIBS) -o $@

clean: 
	rm -rf *.so *.o