#ifndef JAKOPTER_VIDEO_H
#define JAKOPTER_VIDEO_H


#include "common.h"

#define VIDEO_TIMEOUT 4
#define BASE_VIDEO_BUF_SIZE 1024
#define PORT_VIDEO		5555

/**
* Simple structure to hold a decoded video frame.
* The frame is assumed to be in the YUV420p image format.
*/
typedef struct jakopter_video_frame_t {
	int w, h;
	size_t size;
	uint8_t* pixels;
} jakopter_video_frame_t;

/** Video processing API */

struct jakopter_frame_processing {
	/** \brief callback to which is sent every decoded frame.
	  * \param frame buffer containing the raw frame data, encoded in YUV420p.
	  * 	A value of NULL for this parameter means the video stream has ended.
	  * \param width frame width
	  * \param height frame height
	  * \param size size of the buffer in bytes
	  * \return the return value of the callback will be checked by the decoding routine.
	  * 	LESS THAN 0 : the video thread will stop.
	  * 	Anything else : no effect.
	  */
	int (*callback)(uint8_t* frame,
		int width,
		int height,
		int size);
	/*initialize and clean the processing module used by the callback, if needed.
	Can be NULL.*/
	int (*init)(void);
	void (*clean)(void);
};

/** Drawing API*/
struct jakopter_drawing {
	int (*draw_icon)(char* path,
		int x,
		int y,
		int width,
		int height);
	void (*remove)(int id);
	void (*resize)(int id,
		int x,
		int y);
	void (*move)(int id,
		int width,
		int height);
};

int jakopter_draw_icon(char *p, int x, int y, int w, int h);

void jakopter_draw_remove(int id);

void jakopter_draw_resize(int id, int width, int height);
void jakopter_draw_move(int id, int x, int y);

/**
  * Start the thread which receive video packets on port 5555
*/
int jakopter_init_video();
/*
Close the connection and stop the threads
*/
int jakopter_stop_video();
/*
Ask the video thread to stop, but don't wait for it. Shouldn't be called by the user.
*/
int video_set_stopped();

#endif

