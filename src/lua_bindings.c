#include "drone.h"
#include "navdata.h"
#ifdef WITH_VIDEO
#include "video.h"
#endif
#ifdef WITH_LEAP
#include "leapdata.h"
#endif
#include "com_channel.h"
#include "com_master.h"
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

#ifdef WITH_VIDEO
int jakopter_init_video_lua(lua_State* L) {
	lua_pushnumber(L, jakopter_init_video());
	return 1;
}

int jakopter_stop_video_lua(lua_State* L) {
	lua_pushnumber(L, jakopter_stop_video());
	return 1;
}
#endif

#ifdef WITH_LEAP
int jakopter_connect_leap_lua(lua_State* L) {
	lua_pushnumber(L, jakopter_connect_leap());
	return 1;
}

int jakopter_disconnect_leap_lua(lua_State* L) {
	lua_pushnumber(L, jakopter_disconnect_leap());
	return 1;
}
#endif

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

int jakopter_stay_lua(lua_State* L){
	lua_pushnumber(L, jakopter_stay());
	return 1;
}

int jakopter_emergency_lua(lua_State* L)
{
	lua_pushnumber(L, jakopter_emergency());
	return 1;
}

int jakopter_com_create_channel_lua(lua_State* L){
	lua_Integer s = luaL_checkinteger(L, 1);
	luaL_argcheck(L, s > 0, 1, "Channel size must be > 0");
	//store the cc pointer as user data, so that we can assign our custom metatable to it.
	jakopter_com_channel_t** cc = lua_newuserdata(L, sizeof(jakopter_com_channel_t*));
	luaL_getmetatable(L, "jakopter.com_channel");
	lua_setmetatable(L, -2);
	//actually create the channel. Raise an error if it fails.
	*cc = jakopter_com_create_channel((size_t)s);
	if(*cc == NULL)
		return luaL_error(L, "Failed to create a com_channel of size %d", s);

	return 1;
}
int jakopter_com_destroy_channel_lua(lua_State* L){
	jakopter_com_channel_t** cc = check_com_channel(L);
	jakopter_com_destroy_channel(cc);
	if(*cc != NULL)
		return luaL_error(L, "Failed to destroy the com_channel");

	return 0;
}
int jakopter_com_get_channel_lua(lua_State* L) {
	lua_Integer id = luaL_checkinteger(L, 1);

	jakopter_com_channel_t** cc = lua_newuserdata(L, sizeof(jakopter_com_channel_t*));
	luaL_getmetatable(L, "jakopter.com_channel");
	lua_setmetatable(L, -2);

	*cc = jakopter_com_get_channel(id);
	return 1;
}
int jakopter_com_read_int_lua(lua_State* L) {
	jakopter_com_channel_t** cc = check_com_channel(L);
	lua_Integer offset = luaL_checkinteger(L, 2);

	lua_pushnumber(L, jakopter_com_read_int(*cc, offset));
	return 1;
}
int jakopter_com_read_float_lua(lua_State* L) {
	jakopter_com_channel_t** cc = check_com_channel(L);
	lua_Integer offset = luaL_checkinteger(L, 2);

	lua_pushnumber(L, jakopter_com_read_float(*cc, offset));
	return 1;
}
int jakopter_com_write_int_lua(lua_State* L) {
	jakopter_com_channel_t** cc = check_com_channel(L);
	lua_Integer offset = luaL_checkinteger(L, 2);
	lua_Integer value = luaL_checkinteger(L, 3);

	jakopter_com_write_int(*cc, offset, value);
	return 0;
}
int jakopter_com_write_float_lua(lua_State* L) {
	jakopter_com_channel_t** cc = check_com_channel(L);
	lua_Integer offset = luaL_checkinteger(L, 2);
	lua_Integer value = luaL_checknumber(L, 3);

	jakopter_com_write_float(*cc, offset, value);
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
#ifdef WITH_VIDEO
	{"connect_video", jakopter_init_video_lua},
	{"stop_video", jakopter_stop_video_lua},
#endif
#ifdef WITH_LEAP	
	{"connect_leap", jakopter_connect_leap_lua},
	{"disconnect_leap", jakopter_disconnect_leap_lua},
#endif
	{"is_flying", jakopter_is_flying_lua},
	{"height", jakopter_height_lua},
	{"reinit", jakopter_reinit_lua},
	{"ftrim", jakopter_ftrim_lua},
	{"calib", jakopter_calib_lua},
	{"stay", jakopter_stay_lua},
	{"emergency", jakopter_emergency_lua},
	{"create_cc", jakopter_com_create_channel_lua},
	{"destroy_cc", jakopter_com_destroy_channel_lua},
	{"get_cc", jakopter_com_get_channel_lua},
	{"read_int", jakopter_com_read_int_lua},
	{"read_float", jakopter_com_read_float_lua},
	{"write_int", jakopter_com_write_int_lua},
	{"write_float", jakopter_com_write_float_lua},
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

