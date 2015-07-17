/* Jakopter
 * Copyright © 2014 - 2015 Thibaud Hulin, Alexandre Mazure
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
#ifndef JAKOPTER_VIDEO_OPENCV_H
#define JAKOPTER_VIDEO_OPENCV_H

int video_oc_init();
void video_oc_destroy();
int video_oc_process(uint8_t*, int, int, int);

#endif
