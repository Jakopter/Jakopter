#include "video_queue.h"
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

const jakopter_video_frame_t VIDEO_QUEUE_END = {0, 0, 0, NULL};

//the single frame of the queue
jakopter_video_frame_t myFrame;
//is the queue empty ?
bool isEmpty = true;
//condition to wait on the queue to be replenished
pthread_cond_t condEmpty = PTHREAD_COND_INITIALIZER;
//mutex to make sure the queue is accessed atomically
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//buffer that will hold the frame's raw data once it's been pulled for processing
uint8_t* processBuffer = NULL;
//if the frame size increases, we need to reallocate the process buffer.
bool needRealloc = false;

void video_queue_init()
{
	myFrame = VIDEO_QUEUE_END;
	
	isEmpty = true;
	processBuffer = NULL;
	needRealloc = false;
}

void video_queue_free()
{
	if(processBuffer != NULL)
		free(processBuffer);
	processBuffer = NULL;
}

void video_queue_push_frame(const jakopter_video_frame_t* frame)
{
pthread_mutex_lock(&mutex);
	//frame size increase ? Prepare to realloc when the frame will be pulled
	if(frame->size > myFrame.size)
		needRealloc = true;
	/*Simply copy the structure. Don't copy the raw pixel data to save time.*/
	myFrame = *frame;
	/*if the queue was previously empty, send a signal
	to the possibly waiting processing thread so it can go on*/
	if(isEmpty) {
		isEmpty = false;
		pthread_cond_signal(&condEmpty);
	}
pthread_mutex_unlock(&mutex);
}


int video_queue_pull_frame(jakopter_video_frame_t* dest)
{
pthread_mutex_lock(&mutex);
	//if the queue is empty, wait for a frame to be pushed
	while(isEmpty)
		pthread_cond_wait(&condEmpty, &mutex);
	//reallocate memory for the output buffer if needed
	if(needRealloc) {
		free(processBuffer);
		processBuffer = malloc(myFrame.size);
		if(processBuffer == NULL) {
			pthread_mutex_unlock(&mutex);
			return -1;
		}
	}
	//copy ALL of the frame's data, including the raw pixels
	*dest = myFrame;
	dest->pixels = memcpy(processBuffer, myFrame.pixels, myFrame.size);
	//there's only one element in the queue, so it's always empty after pulling it.
	isEmpty = true;
pthread_mutex_unlock(&mutex);
}
