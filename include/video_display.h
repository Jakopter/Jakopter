#ifndef JAKOPTER_VIDEO_DISPLAY_H
#define JAKOPTER_VIDEO_DISPLAY_H

#include <stdint.h>

#define FONT_PATH "../../resources/FreeSans.ttf"
//size of the input com channel for this module
#define DISPLAY_COM_IN_SIZE 32

/**
* "Got frame" callback.
* Fills the texture with the given frame, and displays it on the window.
*/
int video_display_frame(uint8_t* frame, int width, int height, int size);

/**
* Initialize the display module with a default window size.
*/
int video_display_init();

/**
* Free the memory associated with the module
*/
void video_display_clean();

/**
* Navdata values that are to be received by the display module
*/
enum video_nav_infos {
	VIDEO_BAT,
	VIDEO_ALT,
	/*VIDEO_THETA,
	VIDEO_PHI,
	VIDEO_PSI,
	VIDEO_VX,
	VIDEO_VY,
	VIDEO_VZ,*/
	VIDEO_NB_NAV_INFOS
};

#endif
