#ifndef JAKOPTER_DRONE_H
#define JAKOPTER_DRONE_H

#include "common.h"
#include "navdata.h"
#include "com_channel.h"
#include "com_master.h"

#define PACKET_SIZE		256
#define PORT_CMD		5556

/* 30 ms in ns */
#define TIMEOUT_CMD      30000000
#define NAVDATA_ATTEMPT  10
#define HEIGHT_THRESHOLD 500

/* Max number of digit into an integer */
#define SIZE_INT		11
#define SIZE_TYPE		10
#define SIZE_ARG		30
#define ARGS_MAX 		7

#define HEAD_REF 		"REF"
#define HEAD_PCMD 		"PCMD"
#define HEAD_CONFIG 	"CONFIG"
#define HEAD_CTRL 		"CTRL"
#define HEAD_COM_WATCHDOG "COMWDG"
#define HEAD_FTRIM		"FTRIM"
#define HEAD_CALIB		"CALIB"


int jakopter_connect();
int jakopter_takeoff();
int jakopter_land();
int jakopter_emergency();
int jakopter_reinit();
int jakopter_disconnect();
int jakopter_rotate_left(float speed);
int jakopter_rotate_right(float speed);
int jakopter_forward(float speed);
int jakopter_backward(float speed);
int jakopter_up(float speed);
int jakopter_down(float speed);
int jakopter_move(float l_to_r, float f_to_b, float vertical_speed, float angular_speed);
int jakopter_stay();

//DEBUG
int jakopter_flat_trim();
int jakopter_calib();
//Used by navdata
int init_navdata_bootstrap();
int init_navdata_ack();


#endif