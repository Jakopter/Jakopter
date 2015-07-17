/* Jakopter
 * Copyright © 2014 - 2015 Hector Labanca
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
#ifndef JAKOPTER_VIDEO_QUEUE_H
#define JAKOPTER_VIDEO_QUEUE_H

#include "video.h"

/**
* Queue structure to hold decoded video frames waiting to be processed
* by the video processing thread.
* It is intended for use within the video module only.
*/

/*push this frame on the queue to tell the processing thread to stop*/
extern const jakopter_video_frame_t VIDEO_QUEUE_END;

/**
* \brief Set the default values for the queue
*/
void video_queue_init();

/**
* \brief Free the memory allocated for the queue
*/
void video_queue_free();

/**
* \brief Place a video frame at the tail of the queue.
*		The pixel data is NOT copied.
*/
void video_queue_push_frame(const jakopter_video_frame_t* frame);

/**
* \brief Get the frame at the head of the queue.
*		The frame is removed from the queue, its pixel data is copied
*		to the area pointed by dest.
* \returns 0 on success, -1 otherwise.
*/
int video_queue_pull_frame(jakopter_video_frame_t* dest);

#endif

