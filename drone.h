#ifndef JAKOPTER_DRONE_H
#define JAKOPTER_DRONE_H

#include "common.h"
#include "lauxlib.h"
#include "lua.h"

#define PACKET_SIZE		256
#define PORT_CMD		5556

#define TIMEOUT_CMD		30000


int jakopter_connect(lua_State* L);
int jakopter_takeoff(lua_State* L);
int jakopter_land(lua_State* L);
int jakopter_disconnect(lua_State* L);
//DEBUG
int jakopter_get_no_sq(lua_State* L);


#endif