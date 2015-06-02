#ifndef COORDS_H
#define COORDS_H

#include "common.h"
#include "com_channel.h"
#include "com_master.h"

#define COORDS_INTERVAL 	1/3 // interval in seconds

#define COORDS_FILENAME "/tmp/jakopter_coords.txt"

int coords_connect();
int coords_disconnect();


#endif