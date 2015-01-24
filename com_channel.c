#include "com_channel.h"
#include <stdlib.h>
#include <pthread.h>
#include <time.h>


struct jakopter_com_channel_t {
	//prevent concurrent access
	pthread_mutex_t mutex;
	//time at which the structure was created
	clock_t init_time;
	//time in at which data was last written in the buffer (initially init_time)
	clock_t last_write_time;
	//size of the com buffer. Fixed at init.
	size_t buf_size;
	//com buffer where user data will be stored.
	void* buffer;
};


jakopter_com_channel_t* jakopter_init_com_channel(size_t size)
{
	//maybe we should allocate on the stack and copy the struct on return instead ?
	jakopter_com_channel_t* cc = malloc(sizeof(jakopter_com_channel_t));
	if(cc == NULL)
		return NULL;
	
	//initialize the structure's fields
	int error = pthread_mutex_init(&cc->mutex, NULL);
	cc->init_time = clock();
	cc->last_write_time = cc->init_time;
	cc->buf_size = size;
	cc->buffer = malloc(size);
	
	//check allocation failure
	if(cc->buffer == NULL || error) {
		free(cc);
		return NULL;
	}
	
	return cc;
}

void jakopter_destroy_com_channel(jakopter_com_channel_t** cc)
{
	if(cc != NULL && *cc != NULL) {
		pthread_mutex_destroy(&(*cc)->mutex);
		free((*cc)->buffer);
		free(*cc);
		*cc = NULL;
	}
}


void jakopter_write_int(jakopter_com_channel_t* cc, size_t offset, int value)
{

}

void jakopter_write_char(jakopter_com_channel_t* cc, size_t offset, char value)
{

}

void jakopter_write_buf(jakopter_com_channel_t* cc, size_t offset, void* data, size_t size)
{

}


/*******************************************************************
********Functions for reading data from the channel*****************/

int jakopter_read_int(jakopter_com_channel_t* cc, size_t offset)
{
	return 0;
}

char jakopter_read_char(jakopter_com_channel_t* cc, size_t offset)
{
	return 0;
}

//The returned pointer has to be freed by the caller.
void* jakopter_read_buf(jakopter_com_channel_t* cc, size_t offset, size_t size)
{
	return NULL;
}


uint32_t jakopter_get_timestamp(jakopter_com_channel_t* cc)
{
	return 0;
}

