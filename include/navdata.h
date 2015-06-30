#ifndef NAVDATA_H
#define NAVDATA_H

#include "common.h"
#include "com_channel.h"
#include "com_master.h"

#define PORT_NAVDATA	5554
/* interval in seconds*/
#define NAVDATA_INTERVAL 	1/15
#define TAG_DEMO 0
/*Tag for the checksum packet in full mode*/
#define TAG_CKS 0
#define FLOAT_LEN 10
#define DEMO_LEN 4*INT_LEN+6*FLOAT_LEN+10

struct navdata_option {
	uint16_t  tag;
	uint16_t  size;
	uint8_t   data[];
};


struct navdata {
	uint32_t    header; //Always 88776655
	uint32_t    ardrone_state; //Bit mask
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

	float32_t  vx;                     /* UAV's estimated linear velocity in  */
	float32_t  vy;                     /* UAV's estimated linear velocity in  */
	float32_t  vz;                     /* UAV's estimated linear velocity in  */

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

/**
  * \brief Start navdata thread
  * \param drone_ip used by simulator, if you are with a real drone, set it to NULL
  * \return 0 if success, -1 if error
  */
int navdata_connect();
/**
  * \brief Stop navdata thread.
  * \return the pthread_join value or -1 if communication already stopped.
  */
int navdata_disconnect();
/**
  * \return a boolean
  */
int jakopter_is_flying();
/**
  * \return the battery charge in percentage or -1 if navdata are not received
  */
int jakopter_battery();
/**
  * \return the height in millimeters or -1 if navdata are not received
  */
int jakopter_height();
/**
  * \return the sequence number of navdata
  */
int navdata_no_sq();
/**
  * \brief Print the content of received navdata
  */
void debug_navdata_demo();
/**
  * \brief Return the timestamp of the last request: is_flying, etc.
  * Must be called _after_ the request, so be sure to store the result of the request before.
  * \return a string containing the timestamp under this format: "sec.nsec:" or NULL if stopped
  */
const char* jakopter_navdata_timestamp();
/**
  * \brief Return all the data stored with a timestamp.
  * \return a string containing the data under this format: "sec.nsec:data1 data2 " or NULL if stopped
  */
const char* jakopter_log_navdata();

#endif
