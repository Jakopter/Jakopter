#ifndef DRONE_H
#define DRONE_H

#include <stdio.h>
#include <stdlib.h>
#include "lauxlib.h"
#include "lua.h"

#define PACKET_SIZE		256
#define PORT_CMD		5556
#define PORT_VIDEO		5555
#define PORT_NAVDATA	5554
#define TIMEOUT_CMD		30000
#define TIMEOUT_NAVDATA	30000
#define WIFI_ARDRONE_IP	"192.168.1.1"

int jakopter_connect(lua_State* L);
int jakopter_takeoff(lua_State* L);
int jakopter_land(lua_State* L);
int jakopter_disconnect(lua_State* L);
//DEBUG
int jakopter_get_no_sq(lua_State* L);


#endif