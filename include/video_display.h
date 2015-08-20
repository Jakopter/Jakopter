/* Jakopter
 * Copyright © 2014 - 2015 Thibaud Hulin, Thibaut Rousseau, Hector Labanca
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
#ifndef JAKOPTER_VIDEO_DISPLAY_H
#define JAKOPTER_VIDEO_DISPLAY_H

#include "com_master.h"
#include "utils.h"


/* size of the input com channel for this module*/
#define DISPLAY_COM_IN_SIZE 8*sizeof(int)
#define DISPLAY_COM_OUT_SIZE 1*sizeof(int)

enum display_processes {
	NOTHING,
	BLUE_PERCENT
};

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
* \brief Free the memory associated with the module.
* Clean the display context : close the window and clean SDL structures.
*/
void video_display_destroy();


/**
* \brief Draw an icon on the video display. width and height are facultative, set it to 0.
  * \param path the relative path to the icon
  * \param x in pixels from left
  * \param y in pixels from top
  * \param width in pixels, optional
  * \param height in pixels, optional
  * \return the icon id
  */
int display_draw_icon(const char *path, int x, int y, int width, int height);
/**
  * \brief Draw a string on the video display.
  * \param string the text to print
  * \param x in pixels from left
  * \param y in pixels from top
  * \return the icon id
  */
int display_draw_text(const char *string, int x, int y);
/**
* \brief Free the memory associated with the graphic element.
*/
void display_graphic_remove(int id);
/**
  * \brief Resize a graphic element on the display window.
  * \param id the id returned by display_draw_...
  * \param width in pixels
  * \param height in pixels
  */
void display_graphic_resize(int id, int width, int height);
/**
  * \brief Move a graphic element on the display window.
  * \param id the id returned by display_draw_...
  * \param x in pixels from left
  * \param y in pixels from top
  */
void display_graphic_move(int id, int x, int y);

/**
* \brief Navdata values that are going to be received by the display module
*/
#define VIDEO_ICON 1
#define VIDEO_TEXT 2
#define DISPLAY_CYCLES 1
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
