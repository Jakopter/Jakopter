#include "drone.h"
#include "navdata.h"
#ifdef WITH_VIDEO
#include "video.h"
#endif
#include "com_channel.h"
#include "com_master.h"
//pour le yield
#include <sched.h>
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
	float angular_speed = luaL_checknumber(L, 1);

	lua_pushnumber(L, jakopter_rotate_left(angular_speed));
	return 1;
}

int jakopter_rotate_right_lua(lua_State* L) {
	float angular_speed = luaL_checknumber(L, 1);

	lua_pushnumber(L, jakopter_rotate_right(angular_speed));
	return 1;
}

int jakopter_forward_lua(lua_State* L) {
	float speed = luaL_checknumber(L, 1);

	lua_pushnumber(L, jakopter_forward(speed));
	return 1;
}

int jakopter_backward_lua(lua_State* L) {
	float speed = luaL_checknumber(L, 1);

	lua_pushnumber(L, jakopter_backward(speed));
	return 1;
}

int jakopter_up_lua(lua_State* L) {
	float speed = luaL_checknumber(L, 1);

	lua_pushnumber(L, jakopter_up(speed));
	return 1;
}

int jakopter_down_lua(lua_State* L) {
	float speed = luaL_checknumber(L, 1);

	lua_pushnumber(L, jakopter_down(speed));
	return 1;
}

int jakopter_disconnect_lua(lua_State* L) {
	lua_pushnumber(L, jakopter_disconnect());
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
	if(*cc == NULL)
		return luaL_error(L, "Failed to retrieve com_channel of id %d", id);

	return 1;
}
/**
* \brief Read from a com channel.
* \param id of the com channel to read from.
* \param offset of the data to read in the channel.
*/
int jakopter_com_read_int_lua(lua_State* L) {
	//jakopter_com_channel_t** cc = check_com_channel(L);
	lua_Integer id = luaL_checkinteger(L, 1);
	lua_Integer offset = luaL_checkinteger(L, 2);

	jakopter_com_channel_t* cc = jakopter_com_get_channel(id);
	if(cc == NULL)
		return luaL_error(L, "com_channel of id %d doesn't exist", id);

	lua_pushnumber(L, jakopter_com_read_int(cc, offset));
	return 1;
}
int jakopter_com_read_float_lua(lua_State* L) {
	//jakopter_com_channel_t** cc = check_com_channel(L);
	lua_Integer id = luaL_checkinteger(L, 1);
	lua_Integer offset = luaL_checkinteger(L, 2);

	jakopter_com_channel_t* cc = jakopter_com_get_channel(id);
	if(cc == NULL)
		return luaL_error(L, "com_channel of id %d doesn't exist", id);

	lua_pushnumber(L, jakopter_com_read_float(cc, offset));
	return 1;
}
/**
* \brief Write to a com channel.
* \param id of the com channel to write into.
* \param offset of the data to write in the channel.
* \param value to be written at the given offset.
*/
int jakopter_com_write_int_lua(lua_State* L) {
	//jakopter_com_channel_t** cc = check_com_channel(L);
	lua_Integer id = luaL_checkinteger(L, 1);
	lua_Integer offset = luaL_checkinteger(L, 2);
	lua_Integer value = luaL_checkinteger(L, 3);

	jakopter_com_channel_t* cc = jakopter_com_get_channel(id);
	if(cc == NULL)
		return luaL_error(L, "com_channel of id %d doesn't exist", id);

	jakopter_com_write_int(cc, offset, value);
	return 0;
}
int jakopter_com_write_float_lua(lua_State* L) {
	//jakopter_com_channel_t** cc = check_com_channel(L);
	lua_Integer id = luaL_checkinteger(L, 1);
	lua_Integer offset = luaL_checkinteger(L, 2);
	lua_Integer value = luaL_checknumber(L, 3);

	jakopter_com_channel_t* cc = jakopter_com_get_channel(id);
	if(cc == NULL)
		return luaL_error(L, "com_channel of id %d doesn't exist", id);

	jakopter_com_write_float(cc, offset, value);
	return 0;
}
int jakopter_com_get_timestamp_lua(lua_State* L) {
	lua_Integer id = luaL_checkinteger(L, 1);

	jakopter_com_channel_t* cc = jakopter_com_get_channel(id);
	if(cc == NULL)
		return luaL_error(L, "com_channel of id %d doesn't exist", id);

	lua_pushnumber(L, jakopter_com_get_timestamp(cc));
	return 1;
}

int usleep_lua(lua_State* L) {
	lua_Integer duration = luaL_checkinteger(L, 1);
	usleep(duration);
	return 0;
}
int yield_lua(lua_State* L) {
	sched_yield();
	return 0;
}
/**
* Cleanup function called when the library is unloaded.
* Makes sure all threads started by the library are terminated.
*/
int jakopter_cleanup_lua(lua_State* L) {
#ifdef WITH_VIDEO
	jakopter_stop_video();
#endif
	jakopter_disconnect();
	return 0;
}

//enregistrer les fonctions pour lua
//ou luaL_reg
static const luaL_Reg jakopterlib[] = {
	{"connect", jakopter_connect_lua},
	{"takeoff", jakopter_takeoff_lua},
	{"land", jakopter_land_lua},
	{"left", jakopter_rotate_left_lua},
	{"right", jakopter_rotate_right_lua},
	{"forward", jakopter_forward_lua},
	{"backward", jakopter_backward_lua},
	{"up", jakopter_up_lua},
	{"down", jakopter_down_lua},
	{"disconnect", jakopter_disconnect_lua},
#ifdef WITH_VIDEO
	{"connect_video", jakopter_init_video_lua},
	{"stop_video", jakopter_stop_video_lua},
#endif
	{"is_flying", jakopter_is_flying_lua},
	{"height", jakopter_height_lua},
	{"reinit", jakopter_reinit_lua},
	{"ftrim", jakopter_ftrim_lua},
	{"calib", jakopter_calib_lua},
	{"move", jakopter_move_lua},
	{"stay", jakopter_stay_lua},
	{"emergency", jakopter_emergency_lua},
	//we don't need to create/destroy channels in lua.
/*	{"create_cc", jakopter_com_create_channel_lua},
	{"destroy_cc", jakopter_com_destroy_channel_lua},*/
	//now we directly pass the channel id to the read/write functions, no need to retrieve the channel separately
	//{"get_cc", jakopter_com_get_channel_lua},
	{"cc_read_int", jakopter_com_read_int_lua},
	{"cc_read_float", jakopter_com_read_float_lua},
	{"cc_write_int", jakopter_com_write_int_lua},
	{"cc_write_float", jakopter_com_write_float_lua},
	{"cc_get_timestamp", jakopter_com_get_timestamp_lua},
	{"usleep", usleep_lua},
	{"yield", yield_lua},
	{NULL, NULL}
};

/**
* Create a metatable holding the cleanup function
* as the garbage collection event.
* Store a userdata with this metatable in the registry,
* so that it gets garbage collected when Lua exits,
* allowing the cleanup function to be executed.
*/
int create_cleanup_udata(lua_State* L) {
	//metatable with cleanup method for the lib
	luaL_newmetatable(L, "jakopter.cleanup");
	//set our cleanup method as the __gc callback
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, jakopter_cleanup_lua);
	lua_settable(L, -3);
	/*use a userdata to hold our cleanup function.
	We don't store anything in it, so its size is meaningless.*/
	lua_pushstring(L, "jakopter.cleanup_dummy");
	lua_newuserdata(L, 4);
	luaL_getmetatable(L, "jakopter.cleanup");
	lua_setmetatable(L, -2);
	//store this dummy data in Lua's registry.
	lua_settable(L, LUA_REGISTRYINDEX);
	return 0;
}

int luaopen_libjakopter(lua_State* L) {
	//the metatable is used for type-checking our custom structs in lua.
	//here, define a table for com channels pointers.
	luaL_newmetatable(L, "jakopter.com_channel");
	/*create the cleanup registry entry so that cleanup will be executed
	when the lib is unloaded.*/
	create_cleanup_udata(L);

	//lua 5.1 et 5.2 incompatibles...
#if LUA_VERSION_NUM <= 501
	luaL_register(L, "jakopter", jakopterlib);
#else
	lua_newtable(L);
	luaL_setfuncs(L, jakopterlib, 0);
#endif
	return 1;
}

