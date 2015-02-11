#include "drone.h"
#include "navdata.h"
#include "video.h"
#include "com_channel.h"
#include "lauxlib.h"
#include "lua.h"

/*
* Lua helper functions
*/
//check whether the first argument is a com channel.
jakopter_com_channel_t** check_com_channel(lua_State* L)
{
	void* arg = luaL_checkudata(L, 1, "jakopter.com_channel");
	luaL_argcheck(L, arg != NULL, 1, "com_channel expexted");
	return (jakopter_com_channel_t**)arg;
}

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

int jakopter_ftrim_lua(lua_State* L){
	lua_pushnumber(L, jakopter_flat_trim());
	return 1;
}

int jakopter_calib_lua(lua_State* L){
	lua_pushnumber(L, jakopter_calib());
	return 1;
}

int jakopter_move_lua(lua_State* L){
	float l = luaL_checknumber(L, 1);
	float f = luaL_checknumber(L, 2);
	float v = luaL_checknumber(L, 3);
	float a = luaL_checknumber(L, 4);

	lua_pushnumber(L, jakopter_move(l,f,v,a));
	return 1;
}

int jakopter_stay_lua(lua_State* L){
	lua_pushnumber(L, jakopter_stay());
	return 1;
}

int jakopter_init_com_channel_lua(lua_State* L){
	lua_Integer s = luaL_checkinteger(L, 1);
	luaL_argcheck(L, s > 0, 1, "Channel size must be > 0");
	//store the cc pointer as user data, so that we can assign our custom metatable to it.
	jakopter_com_channel_t** cc = lua_newuserdata(L, sizeof(jakopter_com_channel_t*));
	luaL_getmetatable(L, "jakopter.com_channel");
	lua_setmetatable(L, -2);
	//actually create the channel. Raise an error if it fails.
	*cc = jakopter_init_com_channel((size_t)s);
	if(*cc == NULL)
		return luaL_error(L, "Failed to create a com_channel of size %d", s);
	
	return 1;	
}

int jakopter_destroy_com_channel_lua(lua_State* L){
	jakopter_com_channel_t** cc = check_com_channel(L);
	jakopter_destroy_com_channel(cc);
	if(*cc != NULL)
		return luaL_error(L, "Failed to destroy the com_channel");
		
	return 0;
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
	{"ftrim", jakopter_ftrim_lua},
	{"calib", jakopter_calib_lua},
	{"move", jakopter_move_lua},
	{"stay", jakopter_stay_lua},
	{"create_cc", jakopter_init_com_channel_lua},
	{"destroy_cc", jakopter_destroy_com_channel_lua},
	{NULL, NULL}
};

int luaopen_libjakopter(lua_State* L) {
	//the metatable is used for type-checking our custom structs in lua.
	//here, define a table for com channels pointers.
	luaL_newmetatable(L, "jakopter.com_channel");
	//lua 5.1 et 5.2 incompatibles...
#if LUA_VERSION_NUM <= 501
	luaL_register(L, "jakopter", jakopterlib);
#else
	lua_newtable(L);
	luaL_setfuncs(L, jakopterlib, 0);
#endif
	return 1;
}

