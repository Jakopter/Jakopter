/* Jakopter
 * Copyright © 2014 - 2015 Hector Labanca, Thibaud Hulin, Thibaut Rousseau
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

#ifndef JAKOPTER_VIDEO_H
#define JAKOPTER_VIDEO_H


#include "common.h"

/* 1 ms per refresh */
#define VIDEO_INTERVAL 1000000
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
	/** \brief initialize and clean the processing module used by the callback, if needed.
	  * Can be NULL.
	  */
	int (*init)(void);
	void (*clean)(void);
	/** \brief Allow to choose between different callbacks at runtime
	  * Can be NULL.
	  */
	void (*set_callback)(int id);
};

/** Drawing API*/
struct jakopter_drawing {
	/** \brief incrust an image on the frame
	  * \param path the path from the jakopter current working directory
	  * \param x horizontal position in pixels
	  * \param y vertical position in pixels
	  * \param width desired width for this icon in pixels
	  * \param height desired height for this icon in pixels
	  * \return an id use to manipulate this icon
	  */
	int (*draw_icon)(const char* path,
		int x,
		int y,
		int width,
		int height);
	/** \brief incrust a text on the frame.
	  * There is interlacing if you delete then create the text just after.
	  * \param path the path from the jakopter current working directory
	  * \param x horizontal position in pixels
	  * \param y vertical position in pixels
	  */
	int (*draw_text)(const char* string,
		int x,
		int y);
	/** \brief remove a graphic element
	  * \param the id of the graphic element
	  */
	void (*remove)(int id);
	/** \brief resize a graphic element. Can be NULL.
	  * \param the id of the graphic element
	  * \param width desired width for this icon in pixels
	  * \param height desired height for this icon in pixels
	  */
	void (*resize)(int id,
		int width,
		int height);
	/** \brief move a graphic element. Can be NULL.
	  * \param the id of the graphic element
	  * \param x horizontal position in pixels
	  * \param y vertical position in pixels
	  */
	void (*move)(int id,
		int x,
		int y);
};

int jakopter_draw_icon(const char *p, int x, int y, int w, int h);
int jakopter_draw_text(const char *s, int x, int y);
void jakopter_draw_remove(int id);
void jakopter_draw_resize(int id, int width, int height);
void jakopter_draw_move(int id, int x, int y);


void jakopter_set_callback(int id);
/**
  * Start the thread which receive video packets on port 5555
*/
int jakopter_init_video();
/*
* \brief Ask the thread to stop with set_stopped, then
* call join_thread, print a message if the thread has already ended.
* \return -1 on error, 0 otherwise.
*/
int jakopter_stop_video();
/**
  * \brief Ask the video thread to stop without joining with it. Shouldn't be called by user.
  * Useful for stopping it from the inside.
  * \return 0 if stopped was 0, 1 if it wasn't.
  */
int video_set_stopped();
/** \brief helper for mutex-protected value*/
int video_is_stopped();
#endif

