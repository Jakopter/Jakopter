#include "com_channel.h"
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>



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


jakopter_com_channel_t* jakopter_com_create_channel(size_t size)
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

void jakopter_com_destroy_channel(jakopter_com_channel_t** cc)
{
	if(cc != NULL && *cc != NULL) {
		pthread_mutex_destroy(&(*cc)->mutex);
		free((*cc)->buffer);
		free(*cc);
		*cc = NULL;
	}
}


void jakopter_com_write_int(jakopter_com_channel_t* cc, size_t offset, int value)
{
	//make sure we won't be writing out of bounds
	if(offset + sizeof(int) > cc->buf_size)
		return;

	pthread_mutex_lock(&cc->mutex);
	//copy the value at the given offset
	int8_t* place = ((int8_t*)cc->buffer) + offset;
	memcpy(place, &value, sizeof(int));
	//we just modified the buffer, so update the timestamp
	cc->last_write_time = clock();
	
	pthread_mutex_unlock(&cc->mutex);
}

void jakopter_com_write_float(jakopter_com_channel_t* cc, size_t offset, float value)
{
	//make sure we won't be writing out of bounds
	if(offset + sizeof(float) > cc->buf_size)
		return;

	pthread_mutex_lock(&cc->mutex);
	//copy the value at the given offset
	uint8_t* place = ((uint8_t*)cc->buffer) + offset;
	memcpy(place, &value, sizeof(float));
	//we just modified the buffer, so update the timestamp
	cc->last_write_time = clock();
	
	pthread_mutex_unlock(&cc->mutex);
}

void jakopter_com_write_char(jakopter_com_channel_t* cc, size_t offset, char value)
{
	//make sure we won't be writing out of bounds
	if(offset + sizeof(char) > cc->buf_size)
		return;

	pthread_mutex_lock(&cc->mutex);
	//copy the value at the given offset
	int8_t* place = ((int8_t*)cc->buffer) + offset;
	memcpy(place, &value, sizeof(char));
	//we just modified the buffer, so update the timestamp
	cc->last_write_time = clock();
	
	pthread_mutex_unlock(&cc->mutex);
}

void jakopter_com_write_buf(jakopter_com_channel_t* cc, size_t offset, void* data, size_t size)
{
	//make sure we won't be writing out of bounds
	if(offset + size > cc->buf_size || data == NULL)
		return;

	pthread_mutex_lock(&cc->mutex);
	//copy the value at the given offset
	int8_t* place = ((int8_t*)cc->buffer) + offset;
	memcpy(place, data, size);
	//we just modified the buffer, so update the timestamp
	cc->last_write_time = clock();
	
	pthread_mutex_unlock(&cc->mutex);
}


/*******************************************************************
********Functions for reading data from the channel*****************/

int jakopter_com_read_int(jakopter_com_channel_t* cc, size_t offset)
{
	//we can't read over the end
	if(offset + sizeof(int) > cc->buf_size)
		return 0;
	
	pthread_mutex_lock(&cc->mutex);
	//retreive the number
	int8_t* place = ((int8_t*)cc->buffer) + offset;
	int result;
	memcpy(&result, place, sizeof(int));
	pthread_mutex_unlock(&cc->mutex);
	
	return result;
}

char jakopter_com_read_char(jakopter_com_channel_t* cc, size_t offset)
{
	//we can't read over the end
	if(offset + sizeof(char) > cc->buf_size)
		return 0;
	
	pthread_mutex_lock(&cc->mutex);
	//retreive the number
	int8_t* place = ((int8_t*)cc->buffer) + offset;
	char result;
	memcpy(&result, place, sizeof(char));
	pthread_mutex_unlock(&cc->mutex);
	
	return result;
}

void* jakopter_com_read_buf(jakopter_com_channel_t* cc, size_t offset, size_t size, void* dest)
{
	//we can't read over the end
	if(offset + size > cc->buf_size || dest == NULL)
		return NULL;

	pthread_mutex_lock(&cc->mutex);
	//retreive the data
	int8_t* place = ((int8_t*)cc->buffer) + offset;
	memcpy(dest, place, size);
	pthread_mutex_unlock(&cc->mutex);
	
	return dest;
}


double jakopter_com_get_timestamp(jakopter_com_channel_t* cc)
{
	pthread_mutex_lock(&cc->mutex);
	//the timestamp is stored in clock ticks, convert it to milliseconds.
	double ts = ((double)(cc->last_write_time - cc->init_time)) / (CLOCKS_PER_SEC / (double)1000);
	pthread_mutex_unlock(&cc->mutex);
	
	return ts;
}

