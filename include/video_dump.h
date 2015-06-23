#ifndef JAKOPTER_VIDEO_PROCESS_H
#define JAKOPTER_VIDEO_PROCESS_H

#include <stdint.h>
/**
* Header for video processing example(s).
* The idea is to put in this file declarations of functions
* that can be used as callbacks by the video decoding routine
* in order to process video frames received from the drone.
*/
//Name of the file where frames will be dumped
#define JAKO_FRAMEDUMP_FILENAME "frames.yuv"
//Number of frames we want to dump into this file. -1 = unlimited.
#define JAKO_FRAMEDUMP_COUNT 250

/** \brief Dump the video frame to a file.
Ends the video thread once it's reached its limit (see below).*/
int jakopter_dumpFrameToFile(uint8_t* frame, int width, int height, int size);


#endif
