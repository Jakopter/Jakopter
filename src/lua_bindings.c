//for yield function
#include <sched.h>
#include <lauxlib.h>
#include <lua.h>

#include "drone.h"
#include "navdata.h"
#include "com_channel.h"
#include "com_master.h"
#ifdef WITH_VIDEO
#include "video.h"
#endif
#ifdef WITH_COORDS
#include "coords.h"
#endif
#ifdef WITH_NETWORK
#include "network.h"
#endif
/*
* Lua helper functions
*/

/*
Lua bindings to user-exposed functions
*/
int jakopter_connect_lua(lua_State* L)
{
	const char* drone_ip;
	int n = lua_gettop(L);

	//const char *drone_ip = luaL_checkstring(L, 1);
	lua_rawgeti(L, LUA_REGISTRYINDEX, luaL_ref(L, LUA_REGISTRYINDEX));
	if (n == 1) {
		drone_ip = lua_tostring(L, -1);
	}
	else
		drone_ip = NULL;

	lua_pushinteger(L, jakopter_connect(drone_ip));
	return 1; //Number of returned values
}

int jakopter_takeoff_lua(lua_State* L)
{
	lua_pushinteger(L, jakopter_takeoff());
	return 1;
}

int jakopter_land_lua(lua_State* L)
{
	lua_pushinteger(L, jakopter_land());
	return 1;
}

int jakopter_rotate_left_lua(lua_State* L)
{
	float angular_speed = luaL_checknumber(L, 1);

	lua_pushinteger(L, jakopter_rotate_left(angular_speed));
	return 1;
}

int jakopter_rotate_right_lua(lua_State* L)
{
	float angular_speed = luaL_checknumber(L, 1);

	lua_pushinteger(L, jakopter_rotate_right(angular_speed));
	return 1;
}

int jakopter_slide_left_lua(lua_State* L)
{
	float speed = luaL_checknumber(L, 1);

	lua_pushinteger(L, jakopter_slide_left(speed));
	return 1;
}

int jakopter_slide_right_lua(lua_State* L)
{
	float speed = luaL_checknumber(L, 1);

	lua_pushinteger(L, jakopter_slide_right(speed));
	return 1;
}

int jakopter_forward_lua(lua_State* L)
{
	float speed = luaL_checknumber(L, 1);

	lua_pushinteger(L, jakopter_forward(speed));
	return 1;
}

int jakopter_backward_lua(lua_State* L)
{
	float speed = luaL_checknumber(L, 1);

	lua_pushinteger(L, jakopter_backward(speed));
	return 1;
}

int jakopter_up_lua(lua_State* L)
{
	float speed = luaL_checknumber(L, 1);

	lua_pushinteger(L, jakopter_up(speed));
	return 1;
}

int jakopter_down_lua(lua_State* L)
{
	float speed = luaL_checknumber(L, 1);

	lua_pushinteger(L, jakopter_down(speed));
	return 1;
}

int jakopter_disconnect_lua(lua_State* L)
{
	lua_pushinteger(L, jakopter_disconnect());
	return 1;
}

int jakopter_reinit_lua(lua_State* L)
{
	lua_pushinteger(L, jakopter_reinit());
	return 1;
}

int jakopter_ftrim_lua(lua_State* L)
{
	lua_pushinteger(L, jakopter_flat_trim());
	return 1;
}

int jakopter_calib_lua(lua_State* L)
{
	lua_pushinteger(L, jakopter_calib());
	return 1;
}

int jakopter_move_lua(lua_State* L)
{
	float left     = luaL_checknumber(L, 1);
	float forward  = luaL_checknumber(L, 2);
	float vertical = luaL_checknumber(L, 3);
	float angular  = luaL_checknumber(L, 4);

	lua_pushinteger(L, jakopter_move(left, forward, vertical, angular));
	return 1;
}

int jakopter_stay_lua(lua_State* L)
{
	lua_pushinteger(L, jakopter_stay());
	return 1;
}

int jakopter_emergency_lua(lua_State* L)
{
	lua_pushinteger(L, jakopter_emergency());
	return 1;
}

int jakopter_log_command_lua(lua_State* L)
{
	lua_pushstring(L, jakopter_log_command());
	return 1;
}

int jakopter_is_flying_lua(lua_State* L)
{
	lua_pushinteger(L, jakopter_is_flying());
	return 1;
}

int jakopter_height_lua(lua_State* L)
{
	lua_pushnumber(L, jakopter_height());
	return 1;
}

int jakopter_battery_lua(lua_State* L)
{
	lua_pushnumber(L, jakopter_battery());
	return 1;
}

int jakopter_log_navdata_lua(lua_State* L)
{
	lua_pushstring(L, jakopter_log_navdata());
	return 1;
}

int usleep_lua(lua_State* L)
{
	lua_Integer duration = luaL_checkinteger(L, 1);
	usleep(duration);
	return 0;
}
int yield_lua(lua_State* L)
{
	sched_yield();
	return 0;
}
/**
* \brief Read from a com channel.
* \param id of the com channel to read from.
* \param offset of the data to read in the channel.
*/
int jakopter_com_read_int_lua(lua_State* L)
{
	lua_Integer id     = luaL_checkinteger(L, 1);
	lua_Integer offset = luaL_checkinteger(L, 2);

	jakopter_com_channel_t* cc = jakopter_com_get_channel(id);
	if (cc == NULL)
		return luaL_error(L, "[~][lua] com_channel of id %d doesn't exist", id);

	lua_pushinteger(L, jakopter_com_read_int(cc, offset));
	return 1;
}
int jakopter_com_read_float_lua(lua_State* L)
{
	lua_Integer id     = luaL_checkinteger(L, 1);
	lua_Integer offset = luaL_checkinteger(L, 2);

	jakopter_com_channel_t* cc = jakopter_com_get_channel(id);
	if (cc == NULL)
		return luaL_error(L, "[~][lua] com_channel of id %d doesn't exist", id);

	lua_pushnumber(L, jakopter_com_read_float(cc, offset));
	return 1;
}
int jakopter_com_read_string_lua(lua_State* L)
{
	lua_Integer id     = luaL_checkinteger(L, 1);
	lua_Integer offset = luaL_checkinteger(L, 2);

	jakopter_com_channel_t* cc = jakopter_com_get_channel(id);
	if (cc == NULL)
		return luaL_error(L, "[~][lua] com_channel of id %d doesn't exist", id);
	size_t size = jakopter_com_read_int(cc, offset);
	char* string = malloc(size);
	jakopter_com_read_buf(cc, offset+4, size, string);
	if (size == 0)
		lua_pushstring(L, NULL);
	else
		lua_pushstring(L, string);
	free(string);
	return 1;
}
/**
* \brief Write to a com channel.
* \param id of the com channel to write into.
* \param offset of the data to write in the channel.
* \param value to be written at the given offset.
*/
int jakopter_com_write_int_lua(lua_State* L)
{
	lua_Integer id     = luaL_checkinteger(L, 1);
	lua_Integer offset = luaL_checkinteger(L, 2);
	lua_Integer value  = luaL_checkinteger(L, 3);

	jakopter_com_channel_t* cc = jakopter_com_get_channel(id);
	if (cc == NULL)
		return luaL_error(L, "[~][lua] com_channel of id %d doesn't exist", id);

	jakopter_com_write_int(cc, offset, value);
	return 0;
}
int jakopter_com_write_float_lua(lua_State* L)
{
	lua_Integer id     = luaL_checkinteger(L, 1);
	lua_Integer offset = luaL_checkinteger(L, 2);
	float value  = luaL_checknumber(L, 3);

	jakopter_com_channel_t* cc = jakopter_com_get_channel(id);
	if (cc == NULL)
		return luaL_error(L, "[~][lua] com_channel of id %d doesn't exist", id);

	jakopter_com_write_float(cc, offset, value);
	return 0;
}
int jakopter_com_write_string_lua(lua_State* L)
{
	lua_Integer id     = luaL_checkinteger(L, 1);
	lua_Integer offset = luaL_checkinteger(L, 2);
	const char *string = luaL_checkstring(L, 3);

	jakopter_com_channel_t* cc = jakopter_com_get_channel(id);
	if (cc == NULL)
		return luaL_error(L, "[~][lua] com_channel of id %d doesn't exist", id);

	jakopter_com_write_int(cc, offset, strlen(string)+1);
	jakopter_com_write_buf(cc, offset+4, (void*)string, strlen(string)+1);
	return 0;
}
int jakopter_com_get_timestamp_lua(lua_State* L)
{
	lua_Integer id = luaL_checkinteger(L, 1);

	jakopter_com_channel_t* cc = jakopter_com_get_channel(id);
	if (cc == NULL)
		return luaL_error(L, "[~][lua] com_channel of id %d doesn't exist", id);

	lua_pushnumber(L, jakopter_com_get_timestamp(cc));
	return 1;
}

#ifdef WITH_VIDEO
int jakopter_init_video_lua(lua_State* L)
{
	lua_pushinteger(L, jakopter_init_video(NULL));
	return 1;
}

int jakopter_stop_video_lua(lua_State* L)
{
	lua_pushinteger(L, jakopter_stop_video());
	return 1;
}
int jakopter_switch_camera_lua(lua_State* L)
{
	lua_Integer id = luaL_checkinteger(L, 1);

	lua_pushinteger(L, jakopter_switch_camera(id));
	return 1;
}
int jakopter_set_callback_lua(lua_State* L)
{
	lua_Integer id = luaL_checkinteger(L, 1);

	jakopter_set_callback(id);
	return 1;
}
/** Drawing API */
int jakopter_draw_icon_lua(lua_State* L)
{
	const char *path   = luaL_checkstring(L, 1);
	lua_Integer x      = luaL_checkinteger(L, 2);
	lua_Integer y      = luaL_checkinteger(L, 3);
	lua_Integer width  = luaL_checkinteger(L, 4);
	lua_Integer height = luaL_checkinteger(L, 5);
	if (path == NULL || strncmp(path, "", 1) == 0)
		return luaL_error(L, "[~][lua] the path can't be an empty string");

	lua_pushinteger(L, jakopter_draw_icon(path, x, y, width, height));
	return 1;
}

int jakopter_draw_text_lua(lua_State* L)
{
	const char *string = luaL_checkstring(L, 1);
	lua_Integer x      = luaL_checkinteger(L, 2);
	lua_Integer y      = luaL_checkinteger(L, 3);
	if (string == NULL || strncmp(string, "", 1) == 0)
		return luaL_error(L, "[~][lua] the string can't be empty");

	lua_pushinteger(L, jakopter_draw_text(string, x, y));
	return 1;
}

int jakopter_draw_remove_lua(lua_State* L)
{
	lua_Integer id = luaL_checkinteger(L, 1);

	jakopter_draw_remove(id);
	return 0;
}

int jakopter_draw_resize_lua(lua_State* L)
{
	lua_Integer id     = luaL_checkinteger(L, 1);
	lua_Integer width  = luaL_checkinteger(L, 2);
	lua_Integer height = luaL_checkinteger(L, 3);

	jakopter_draw_resize(id, width, height);
	return 0;
}

int jakopter_draw_move_lua(lua_State* L)
{
	lua_Integer id = luaL_checkinteger(L, 1);
	lua_Integer x  = luaL_checkinteger(L, 2);
	lua_Integer y  = luaL_checkinteger(L, 3);

	jakopter_draw_move(id, x, y);
	return 0;
}
#endif

#ifdef WITH_COORDS
int jakopter_init_coords_lua(lua_State* L)
{
	lua_pushinteger(L, jakopter_init_coords());
	return 1;
}
int jakopter_stop_coords_lua(lua_State* L)
{
	lua_pushinteger(L, jakopter_stop_coords());
	return 1;
}
int jakopter_log_coords_lua(lua_State* L)
{
	lua_pushstring(L, jakopter_log_coords());
	return 1;
}
#endif

#ifdef WITH_NETWORK
int jakopter_init_network_lua(lua_State* L)
{
	const char *server_input = luaL_checkstring(L, 1);
	const char *server_output = luaL_checkstring(L, 2);

	lua_pushinteger(L, jakopter_init_network(server_input, server_output));
	return 1;
}
int jakopter_stop_network_lua(lua_State* L)
{
	lua_pushinteger(L, jakopter_stop_network());
	return 1;
}
#endif

/**
* Cleanup function called when the library is unloaded.
* Makes sure all threads started by the library are terminated.
*/
int jakopter_cleanup_lua(lua_State* L)
{
	if(jakopter_is_flying())
		jakopter_land();
#ifdef WITH_VIDEO
	jakopter_stop_video();
#endif
#ifdef WITH_COORDS
	jakopter_stop_coords();
#endif
#ifdef WITH_NETWORK
	jakopter_stop_network();
#endif
	jakopter_disconnect();
	return 0;
}

/*define functions used by lua
or luaL_Reg*/
static const luaL_Reg jakopterlib[] = {
	{"connect", jakopter_connect_lua},
	{"disconnect", jakopter_disconnect_lua},
	{"takeoff", jakopter_takeoff_lua},
	{"land", jakopter_land_lua},
	{"emergency", jakopter_emergency_lua},
	{"left", jakopter_rotate_left_lua},
	{"right", jakopter_rotate_right_lua},
	{"slide_left", jakopter_slide_left_lua},
	{"slide_right", jakopter_slide_right_lua},
	{"forward", jakopter_forward_lua},
	{"backward", jakopter_backward_lua},
	{"up", jakopter_up_lua},
	{"down", jakopter_down_lua},
	{"stay", jakopter_stay_lua},
	{"move", jakopter_move_lua},
	{"reinit", jakopter_reinit_lua},
	{"ftrim", jakopter_ftrim_lua},
	{"calib", jakopter_calib_lua},
	{"log_command", jakopter_log_command_lua},
	{"is_flying", jakopter_is_flying_lua},
	{"battery", jakopter_battery_lua},
	{"height", jakopter_height_lua},
	{"log_navdata", jakopter_log_navdata_lua},
	{"cc_read_int", jakopter_com_read_int_lua},
	{"cc_read_float", jakopter_com_read_float_lua},
	{"cc_read_string",jakopter_com_read_string_lua},
	{"cc_write_int", jakopter_com_write_int_lua},
	{"cc_write_float", jakopter_com_write_float_lua},
	{"cc_write_string",jakopter_com_write_string_lua},
	{"cc_get_timestamp", jakopter_com_get_timestamp_lua},
	{"usleep", usleep_lua},
	{"yield", yield_lua},
#ifdef WITH_VIDEO
	{"connect_video", jakopter_init_video_lua},
	{"stop_video", jakopter_stop_video_lua},
	{"switch_cam", jakopter_switch_camera_lua},
	{"set_callback", jakopter_set_callback_lua},
	{"draw_icon", jakopter_draw_icon_lua},
	{"draw_text", jakopter_draw_text_lua},
	{"draw_remove", jakopter_draw_remove_lua},
	{"draw_resize", jakopter_draw_resize_lua},
	{"draw_move", jakopter_draw_move_lua},
#endif
#ifdef WITH_NETWORK
	{"connect_network", jakopter_init_network_lua},
	{"stop_network", jakopter_stop_network_lua},
#endif
#ifdef WITH_COORDS
	{"connect_coords", jakopter_init_coords_lua},
	{"stop_coords", jakopter_stop_coords_lua},
	{"log_coords", jakopter_log_coords_lua},
#endif
	{NULL, NULL}
};

/**
* Create a metatable holding the cleanup function
* as the garbage collection event.
* Store a userdata with this metatable in the registry,
* so that it gets garbage collected when Lua exits,
* allowing the cleanup function to be executed.
*/
static int create_cleanup_udata(lua_State* L)
{
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

JAKO_EXPORT int luaopen_libjakopter(lua_State* L)
{
	/*the metatable is used for type-checking our custom structs in lua.
	here, define a table for com channels pointers.*/
	luaL_newmetatable(L, "jakopter.com_channel");
	/*create the cleanup registry entry so that cleanup will be executed
	when the lib is unloaded.*/
	create_cleanup_udata(L);

	/* lua 5.1 and 5.2 incompatibles... */
#if LUA_VERSION_NUM <= 501
	luaL_register(L, "jakopter", jakopterlib);
#else
	lua_newtable(L);
	luaL_setfuncs(L, jakopterlib, 0);
#endif
	return 1;
}

