#ifndef JAKOPTER_DRONE_H
#define JAKOPTER_DRONE_H

#include "common.h"
#include "navdata.h"

#define PACKET_SIZE		256
#define PORT_CMD		5556

#define TIMEOUT_CMD		30000

/* max number of digit into an integer */
#define SIZE_INT 		30
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
int jakopter_reinit();
int jakopter_disconnect();
int jakopter_rotate_left();
int jakopter_rotate_right();
int jakopter_forward();
int jakopter_backward();

//DEBUG
int jakopter_get_no_sq();
int jakopter_flat_trim();
int jakopter_calib();
//Used by navdata
int init_navdata_bootstrap();
int init_navdata_ack();


#endif