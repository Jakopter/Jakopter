/* Jakopter
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
#ifndef JAKOPTER_COORDS_H
#define JAKOPTER_COORDS_H

#include "com_channel.h"
#include "com_master.h"
#include "utils.h"

/* Max number of digit into an integer. */

#define COORDS_NREADS 10
#define COORDS_TIMEOUT 2000
#define COORDS_INTERVAL 	1 // interval in seconds
/*size of float digits plus a space and \0*/
#define COORDS_BUF_SIZE (FLOAT_LEN + 1)*COORDS_NREADS + 1
#define COORDS_FILENAME "/tmp/jakopter_coords.txt"

int jakopter_init_coords();
int jakopter_stop_coords();

const char* jakopter_log_coords();


#endif