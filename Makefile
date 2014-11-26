LIBS+= `pkg-config lua libavcodec --libs` -pthread
LDFLAGS+= -shared
CFLAGS+= -c -std=gnu99 -Wall -fpic `pkg-config lua libavcodec --cflags`

SRCS= $(wildcard *.c)
OBJS= $(SRCS:.c=.o)

drone.so: $(OBJS)
	gcc $(LDFLAGS) $^ $(LIBS) -o $@
	
%.o: %.c
	gcc $(CFLAGS) $< -o $@

#use gcc's MMD option to generate header dependencies for source files.	
CFLAGS += -MMD
-include $(OBJS:.o=.d)

clean: 
	rm -rf *.so *.o *.d
