#ifndef COORDS_H
#define COORDS_H

#include "common.h"
#include "com_channel.h"
#include "com_master.h"

#define COORDS_INTERVAL 	1 // interval in seconds
/*size of float digits plus 3 spaces and \0*/
#define COORDS_BUF_SIZE DECIMAL_DIG*3+4
#define COORDS_FILENAME "/tmp/jakopter_coords.txt"

int jakopter_init_coords();
int jakopter_stop_coords();

const char* jakopter_log_coords();


#endif