TEST_EXEC= test_com_channel
TEST_SRCS= test_com_channel.c
TEST_OBJS= test_com_channel.o
CFLAGS= -c -I..


test: $(TEST_EXEC)
	
$(TEST_EXEC): $(TEST_OBJS) ../com_channel.o
	gcc $^ -o $@
	
%.o: %.c
	gcc $(CFLAGS) $< -o $@

