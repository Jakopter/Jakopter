#include "drone.h"
#include "navdata.h"
#include "video.h"
#include "lauxlib.h"
#include "lua.h"

/*
Lua bindings to user-exposed functions
*/
int jakopter_connect_lua(lua_State* L) {
    lua_pushnumber(L, jakopter_connect());
    return 1; //Number of returned values
}

int jakopter_takeoff_lua(lua_State* L) {
    lua_pushnumber(L, jakopter_takeoff());
    return 1;
}

int jakopter_land_lua(lua_State* L) {
    lua_pushnumber(L, jakopter_land());
    return 1;
}

int jakopter_rotate_left_lua(lua_State* L) {
    lua_pushnumber(L, jakopter_rotate_left());
    return 1;
}

int jakopter_rotate_right_lua(lua_State* L) {
    lua_pushnumber(L, jakopter_rotate_right());
    return 1;
}

int jakopter_forward_lua(lua_State* L) {
    lua_pushnumber(L, jakopter_forward());
    return 1;
}

int jakopter_backward_lua(lua_State* L) {
    lua_pushnumber(L, jakopter_backward());
    return 1;
}

int jakopter_disconnect_lua(lua_State* L) {
    lua_pushnumber(L, jakopter_disconnect());
    return 1;
}

int jakopter_get_no_sq_lua(lua_State* L) {
    lua_pushnumber(L, jakopter_get_no_sq());
    return 1;
}

int jakopter_init_video_lua(lua_State* L) {
    lua_pushnumber(L, jakopter_init_video());
    return 1;
}

int jakopter_stop_video_lua(lua_State* L) {
    lua_pushnumber(L, jakopter_stop_video());
    return 1;
}
int jakopter_is_flying_lua(lua_State* L){
    lua_pushnumber(L, jakopter_is_flying());
    return 1;
}
int jakopter_height_lua(lua_State* L){
    lua_pushnumber(L, jakopter_height());
    return 1;
}
int jakopter_reinit_lua(lua_State* L){
    lua_pushnumber(L, jakopter_reinit());
    return 1;
}


//enregistrer les fonctions pour lua
//ou luaL_reg
static const luaL_Reg jakopterlib[] = {
	{"connect", jakopter_connect_lua},
	{"takeoff", jakopter_takeoff_lua},
	{"land", jakopter_land_lua},
	{"left", jakopter_rotate_left},
	{"right", jakopter_rotate_right},
	{"forward", jakopter_forward},
	{"backward", jakopter_backward},
	{"disconnect", jakopter_disconnect_lua},
	{"get_no_sq", jakopter_get_no_sq_lua},
	{"connect_video", jakopter_init_video_lua},
	{"stop_video", jakopter_stop_video_lua},
    {"is_flying", jakopter_is_flying_lua},
    {"height", jakopter_height_lua},
    {"reinit", jakopter_reinit_lua},
	{NULL, NULL}
};

int luaopen_drone(lua_State* L) {
	//lua 5.1 et 5.2 incompatibles...
#if LUA_VERSION_NUM <= 501
	luaL_register(L, "jakopter", jakopterlib);
#else
	lua_newtable(L);
	luaL_setfuncs(L, jakopterlib, 0);
#endif
	return 1;
}
