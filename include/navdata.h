#ifndef NAVDATA_H
#define NAVDATA_H

#include "common.h"
#include "com_channel.h"
#include "com_master.h"
#define PORT_NAVDATA	5554
#define NAVDATA_INTERVAL 	1/15 // interval in seconds
#define TAG_DEMO 0
#define TAG_CKS 0

struct navdata_option {
	uint16_t  tag;
	uint16_t  size;
	uint8_t   data[];
};


struct navdata {
	uint32_t    header;
	uint32_t    ardrone_state;
	uint32_t    sequence; //Sequence number
	bool		vision_defined;

	struct navdata_option  options[1]; //static pointer
};

struct navdata_demo
{
	uint32_t   header;
	uint32_t   ardrone_state;
	uint32_t   sequence;
	bool	   vision_defined;

	uint16_t   tag;
	uint16_t   size;

	uint32_t   ctrl_state;             /* Flying state (landed, flying, hovering, etc.). */
	uint32_t   vbat_flying_percentage; /* Battery voltage filtered (mV) */

	float32_t  theta;                  /* UAV's pitch in milli-degrees */
	float32_t  phi;                    /* UAV's roll  in milli-degrees */
	float32_t  psi;                    /* UAV's yaw   in milli-degrees */

	int32_t    altitude;               /* UAV's altitude in centimeters */

	float32_t  vx;                     /* UAV's estimated linear velocity */
	float32_t  vy;                     /* UAV's estimated linear velocity */
	float32_t  vz;                     /* UAV's estimated linear velocity */

	uint32_t   num_frames; // Useless

	float32_t  detection_camera_rot[9]; // Useless
	float32_t  detection_camera_trans[3]; // Useless
	uint32_t   detection_tag_index; // Useless

	uint32_t   detection_camera_type;  /* Type of tag searched in detection */

	float32_t  drone_camera_rot[9]; // Useless
	float32_t  drone_camera_trans[3]; // Useless
};

union navdata_t {
	struct navdata raw;
	struct navdata_demo demo;
};

int navdata_connect();
int navdata_disconnect();
int jakopter_is_flying();
int jakopter_height();
float jakopter_y_axis();

int navdata_no_sq();

#endif
