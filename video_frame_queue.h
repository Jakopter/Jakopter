#ifndef JAKOPTER_VIDEO_FRAME_QUEUE_H
#define JAKOPTER_VIDEO_FRAME_QUEUE_H

/*
* Structure that holds a video frame.
* Its pixel format is assumed to be YUV420p.
* w, h : dimensions of the frame.
* time : timestamp of the frame (the time at which it was received).
* data : the raw pixel data of the frame.
*/
typedef struct jako_frame_t {
	int w, h;
	unsigned int time;
	uint8_t* data;
} jako_frame_t;

/*
* Push a frame on the queue.
* The pushed frame becomes the new head.
* It should be freed by the caller, if needed.
*/
int video_push_frame(jako_frame_t* frame);

/*
* Get the queue's head, and remove it.
* The memory for both the data and the structure should be freed by the caller.
*/
int video_pull_frame(jako_frame_t* frame);


int video_get_latest_frame(jako_frame_t* frame);
#endif
