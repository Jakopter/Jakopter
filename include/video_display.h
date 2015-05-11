#ifndef JAKOPTER_VIDEO_DISPLAY_H
#define JAKOPTER_VIDEO_DISPLAY_H

#include <stdint.h>

#define FONT_PATH "../../resources/FreeSans.ttf"
/* size of the input com channel for this module*/
#define DISPLAY_COM_IN_SIZE 32

/**
* \brief "Got frame" callback.
* Fills the texture with the given frame, and displays it on the window.
*/
int video_display_process(uint8_t* frame, int width, int height, int size);

/**
* \brief Create the com_channel needed to communicate with the display module.
*/
int video_display_init();

/**
* \brief Free the memory associated with the module
*/
void video_display_destroy();

/**
* \brief Navdata values that are to be received by the display module
*/
enum video_nav_infos {
	VIDEO_BAT,
	VIDEO_ALT,
	VIDEO_ICON,
	/*VIDEO_THETA,
	VIDEO_PHI,
	VIDEO_PSI,
	VIDEO_VX,
	VIDEO_VY,
	VIDEO_VZ,*/
	VIDEO_NB_NAV_INFOS
};

#endif
