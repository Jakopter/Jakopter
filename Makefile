
LIBS+= `pkg-config lua5.2 --libs`  `pkg-config libavcodec --libs` -pthread
LDFLAGS+= -shared
CFLAGS+= -c -std=gnu99 -Wall -fpic `pkg-config lua5.2 --cflags` `pkg-config libavcodec --cflags` 
#LIBS+= `pkg-config lua --libs` `pkg-config libavcodec --libs` -pthread
#LDFLAGS+= -shared
#CFLAGS+= -c -std=gnu99 -Wall -fpic `pkg-config lua --cflags` `pkg-config libavcodec --cflags`

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
