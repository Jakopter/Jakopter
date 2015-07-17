/* Jakopter
 * Copyright © 2014 - 2015 Thibaud Hulin, Hector Labanca
 * Copyright © 2015 ALF@INRIA
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "com_channel.h"


struct jakopter_com_channel_t {
	/*prevent concurrent access*/
	pthread_mutex_t mutex;
	/*time at which the structure was created*/
	clock_t init_time;
	/*time in at which data was last written in the buffer (initially init_time)*/
	clock_t last_write_time;
	/*size of the com buffer. Fixed at init.*/
	size_t buf_size;
	/*com buffer where user data will be stored.*/
	void* buffer;
};


jakopter_com_channel_t* jakopter_com_create_channel(size_t size)
{
	//maybe we should allocate on the stack and copy the struct on return instead ?
	jakopter_com_channel_t* cc = malloc(sizeof(jakopter_com_channel_t));
	void* buffer = malloc(size);
	if (cc == NULL || buffer == NULL) {
		free(cc);
		free(buffer);
		fprintf(stderr, "[com_channel] Error : failed to allocate memory\n");
		return NULL;
	}

	//initialize the structure's fields
	int error = pthread_mutex_init(&cc->mutex, NULL);
	cc->init_time = clock();
	cc->last_write_time = cc->init_time;
	cc->buf_size = size;
	cc->buffer = buffer;

	memset(cc->buffer, 0, size);

	//check allocation failure
	if (error) {
		free(cc);
		free(buffer);
		fprintf(stderr, "[com_channel] Error : failed to initialize mutex\n");
		return NULL;
	}

	return cc;
}

void jakopter_com_destroy_channel(jakopter_com_channel_t** cc)
{
	if (cc != NULL && *cc != NULL) {
		pthread_mutex_destroy(&(*cc)->mutex);
		free((*cc)->buffer);
		free(*cc);
		*cc = NULL;
	}
	else
		fprintf(stderr, "[com_channel] Warning : destroy_channel got a NULL channel\n");
}


JAKO_EXPORT void jakopter_com_write_int(jakopter_com_channel_t* cc, size_t offset, int value)
{
	//debug checks
	if (cc == NULL) {
		fprintf(stderr, "[com_channel] Error : got NULL com channel\n");
		return;
	}
	//make sure we won't be writing out of bounds
	if (offset + sizeof(int) > cc->buf_size) {
		fprintf(stderr, "[com_channel] Error : attempt to write out of bounds\n");
		return;
	}

	pthread_mutex_lock(&cc->mutex);
	//copy the value at the given offset
	int8_t* place = ((int8_t*)cc->buffer) + offset;
	memcpy(place, &value, sizeof(int));
	//we just modified the buffer, so update the timestamp
	cc->last_write_time = clock();

	pthread_mutex_unlock(&cc->mutex);
}

JAKO_EXPORT void jakopter_com_write_float(jakopter_com_channel_t* cc, size_t offset, float value)
{
	//debug checks
	if (cc == NULL) {
		fprintf(stderr, "[com_channel] Error : got NULL com channel\n");
		return;
	}
	//make sure we won't be writing out of bounds
	if (offset + sizeof(float) > cc->buf_size) {
		fprintf(stderr, "[com_channel] Error : attempt to write out of bounds\n");
		return;
	}

	pthread_mutex_lock(&cc->mutex);
	//copy the value at the given offset
	uint8_t* place = ((uint8_t*)cc->buffer) + offset;
	memcpy(place, &value, sizeof(float));
	//we just modified the buffer, so update the timestamp
	cc->last_write_time = clock();
	pthread_mutex_unlock(&cc->mutex);
}

JAKO_EXPORT void jakopter_com_write_char(jakopter_com_channel_t* cc, size_t offset, char value)
{
	//debug checks
	if (cc == NULL) {
		fprintf(stderr, "[com_channel] Error : got NULL com channel\n");
		return;
	}
	//make sure we won't be writing out of bounds
	if (offset + sizeof(char) > cc->buf_size) {
		fprintf(stderr, "[com_channel] Error : attempt to write out of bounds\n");
		return;
	}

	pthread_mutex_lock(&cc->mutex);
	//copy the value at the given offset
	int8_t* place = ((int8_t*)cc->buffer) + offset;
	memcpy(place, &value, sizeof(char));
	//we just modified the buffer, so update the timestamp
	cc->last_write_time = clock();

	pthread_mutex_unlock(&cc->mutex);
}

JAKO_EXPORT void jakopter_com_write_buf(jakopter_com_channel_t* cc, size_t offset, void* data, size_t size)
{
	//debug checks
	if (cc == NULL) {
		fprintf(stderr, "[com_channel] Error : got NULL com channel\n");
		return;
	}
	//make sure we won't be writing out of bounds
	if (offset + size > cc->buf_size) {
		fprintf(stderr, "[com_channel] Error : attempt to write out of bounds\n");
		return;
	}

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

JAKO_EXPORT int jakopter_com_read_int(jakopter_com_channel_t* cc, size_t offset)
{
	//debug checks
	if (cc == NULL) {
		fprintf(stderr, "[com_channel] Error : got NULL com channel\n");
		return 0;
	}
	//we can't read over the end
	if (offset + sizeof(int) > cc->buf_size) {
		fprintf(stderr, "[com_channel] Error : attempt to read out of bounds\n");
		return 0;
	}

	pthread_mutex_lock(&cc->mutex);
	//retreive the number
	int8_t* place = ((int8_t*)cc->buffer) + offset;
	int result;
	memcpy(&result, place, sizeof(int));
	pthread_mutex_unlock(&cc->mutex);

	return result;
}

JAKO_EXPORT float jakopter_com_read_float(jakopter_com_channel_t* cc, size_t offset)
{
	//debug checks
	if (cc == NULL) {
		fprintf(stderr, "[com_channel] Error : got NULL com channel\n");
		return 0;
	}
	//we can't read over the end
	if (offset + sizeof(float) > cc->buf_size) {
		fprintf(stderr, "[com_channel] Error : attempt to read out of bounds\n");
		return 0;
	}

	pthread_mutex_lock(&cc->mutex);
	//retreive the number
	int8_t* place = ((int8_t*)cc->buffer) + offset;
	float result;
	memcpy(&result, place, sizeof(float));
	pthread_mutex_unlock(&cc->mutex);

	return result;
}

JAKO_EXPORT char jakopter_com_read_char(jakopter_com_channel_t* cc, size_t offset)
{
	//debug checks
	if (cc == NULL) {
		fprintf(stderr, "[com_channel] Error : got NULL com channel\n");
		return 0;
	}
	//we can't read over the end
	if (offset + sizeof(char) > cc->buf_size) {
		fprintf(stderr, "[com_channel] Error : attempt to read out of bounds\n");
		return 0;
	}

	pthread_mutex_lock(&cc->mutex);
	//retreive the number
	int8_t* place = ((int8_t*)cc->buffer) + offset;
	char result;
	memcpy(&result, place, sizeof(char));
	pthread_mutex_unlock(&cc->mutex);

	return result;
}

JAKO_EXPORT void* jakopter_com_read_buf(jakopter_com_channel_t* cc, size_t offset, size_t size, void* dest)
{
	//debug checks
	if (cc == NULL) {
		fprintf(stderr, "[com_channel] Error : got NULL com channel\n");
		return NULL;
	}
	//we can't read over the end
	if (offset + size > cc->buf_size) {
		fprintf(stderr, "[com_channel] Error : attempt to read out of bounds\n");
		return NULL;
	}

	pthread_mutex_lock(&cc->mutex);
	//retrieve the data
	int8_t* place = ((int8_t*)cc->buffer) + offset;
	memcpy(dest, place, size);
	pthread_mutex_unlock(&cc->mutex);

	return dest;
}


JAKO_EXPORT double jakopter_com_get_timestamp(jakopter_com_channel_t* cc)
{
	//debug checks
	if (cc == NULL) {
		fprintf(stderr, "[com_channel] Error : got NULL com channel\n");
		return 0;
	}
	pthread_mutex_lock(&cc->mutex);
	//the timestamp is stored in clock ticks, convert it to milliseconds.
	double ts = ((double)(cc->last_write_time - cc->init_time)) / (CLOCKS_PER_SEC / (double)1000);
	pthread_mutex_unlock(&cc->mutex);

	return ts;
}

