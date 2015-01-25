/*
* Unit testing for the com channel module
*/
#include "../com_channel.h"
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define TEST_SIZE 1024
#define NUM_WRITES 20

static bool write_read_test_int();
static bool write_read_test_str();
static bool timestamp_test();
//threads used for the test
void* write_thread(void* args);
void* read_thread(void* args);

jakopter_com_channel_t* testch;

int main(int argc, char *argv[])
{
	printf("Making a %d bytes channel.\n", TEST_SIZE);
	testch = jakopter_init_com_channel(TEST_SIZE);
	if(testch == NULL) {
		printf("Channel creation failed !\n");
		return 1;
	}
	
	printf("Testing int read/write coherence.\n");
	if(write_read_test_int())
		printf("Test OK.\n");
	else
		printf("Test failed !\n");
		
	printf("Testing string read/write coherence.\n");
	if(write_read_test_str())
		printf("Test OK.\n");
	else
		printf("Test failed !\n");
		
	printf("Testing timestamp coherence.\n");
	if(timestamp_test())
		printf("Test OK.\n");
	else
		printf("Test failed !\n");
	
	jakopter_destroy_com_channel(&testch);
	if(testch == NULL)
		printf("Com channel destruction OK.\n");
	else
		printf("Com channel destruction failed !\n");
		
	return 0;
}

/*
* \brief Write something, then read at the same place
		and check the values are equal.
*/
bool write_read_test_int()
{
	int value1 = 5;

	printf("Writing %d\n", value1);
	jakopter_write_int(testch, 1, value1);
	int val1_read = jakopter_read_int(testch, 1);
	if(val1_read == value1)
		printf("Read value matches.\n");
	else {
		printf("Read value mismatch : %d != %d !\n", val1_read, value1);
		return false;
	}
	
	return true;
}

/*
* \brief Test W/R for a string.
*/
bool write_read_test_str()
{
	char str[] = "chaine";
	char* res = NULL;
	
	jakopter_write_buf(testch, 0, str, sizeof(str));
	printf("Wrote : %s\n", str);
	res = jakopter_read_buf(testch, 0, sizeof(str));
	printf("Read : %s\n", res);
	if(res != NULL && strcmp(str, res) == 0) {
		free(res);
		return true;
	}
	else
		return false;
}

/*
* \brief Make sure writing something changes the timestamp.
*/
bool timestamp_test()
{
	int value1 = 5;
	double initial = jakopter_get_timestamp(testch);
	printf("Initial timestamp : %lf. Writing %d\n", initial, value1);
	jakopter_write_int(testch, 0, value1);
	double val1_ts = jakopter_get_timestamp(testch);
	printf("New timestamp : %lf\n", val1_ts);
	if(val1_ts <= initial) {
		printf("Error : new timestamp should be greater than the initial one !\n");
		return false;
	}
	return true;
}

void* write_thread(void* args)
{
	
}

void* read_thread(void* args)
{

}
