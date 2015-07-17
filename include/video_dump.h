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
#ifndef JAKOPTER_VIDEO_DUMP_H
#define JAKOPTER_VIDEO_DUMP_H
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
