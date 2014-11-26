#ifndef JAKOPTER_DRONE_H
#define JAKOPTER_DRONE_H

#include "common.h"

#define PACKET_SIZE		256
#define PORT_CMD		5556

#define TIMEOUT_CMD		30000


int jakopter_connect();
int jakopter_takeoff();
int jakopter_land();
int jakopter_disconnect();
int jakopter_rotate_left();
int jakopter_rotate_right();
int jakopter_forward();
int jakopter_backward();

//DEBUG
int jakopter_get_no_sq();


#endif