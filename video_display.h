#ifndef JAKOPTER_VIDEO_DISPLAY_H
#define JAKOPTER_VIDEO_DISPLAY_H

#include <stdint.h>

/*
* "Got frame" callback.
* Fills the texture with the given frame, and displays it on the window.
*/
int video_display_frame(uint8_t* frame, int width, int height, int size);

#endif
