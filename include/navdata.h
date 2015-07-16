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
#define NAVDATA_NREADS_INT 4
#define NAVDATA_NREADS_FLOAT 6
// 2 spaces
#define DEMO_LEN NAVDATA_NREADS_INT*(INT_LEN+1)+(NAVDATA_NREADS_FLOAT*FLOAT_LEN+1)+2

/* Values in ctrl_state */

/** Main value in the first 16 bits. */
typedef enum {
	DEFAULT,
	INIT,
	LANDED,
	FLY,
	HOVER,
	CTRL_USELESS_1,
	CTRL_USELESS_2,
	TAKEOFF,
	MOVE,
	LAND,
	LOOP
} ctrl_states;

/** Specific value in the last 16 bits. */
typedef enum {
	FLY_OK,
	FLY_LOST_ALT,
	FLY_LOST_ALT_GO_DOWN,
	FLY_ALT_OUT_ZONE,
	COMBINED_YAW,
	BRAKE,
	NO_VISION
} fly_states;

typedef enum {
	HOVER_OK,
	HOVER_YAW,
	HOVER_YAW_LOST_ALT,
	HOVER_YAW_LOST_ALT_GO_DOWN,
	HOVER_ALT_OUT_ZONE,
	HOVER_YAW_ALT_OUT_ZONE,
	HOVER_LOST_ALT,
	HOVER_LOST_ALT_GO_DOWN,
	LOST_COM,
	LOST_COM_LOST_ALT,
	LOST_COM_LOST_ALT_TOO_LONG,
	LOST_COM_ALT_OK,
	MAGNETO_CALIB,
	DEMO
} hover_states;

typedef enum {
	TAKEOFF_GROUND,
	TAKEOFF_AUTO
} takeoff_states;

typedef enum {
	GOTO_OK = 0,
	GOTO_LOST_ALT,
	GOTO_YAW
} move_states;

typedef enum {
	CLOSED_LOOP,
	OPEN_LOOP,
	OPEN_LOOP_FAST
} land_states;

typedef enum {
	IMPULSION,
	OPEN_LOOP_CTRL,
	PLANIF_CTRL
} loop_states;

/* Navdata structs */

struct navdata_option {
	/** type of the packet: DEMO, etc.*/
	uint16_t  tag;
	uint16_t  size;
	uint8_t   data[];
};

struct navdata {
	/** Always 88776655 */
	uint32_t    header;
	/** Bit mask defined in SDK config.h */
	uint32_t    ardrone_state;
	/** Sequence number of the packet */
	uint32_t    sequence;
	/** True: vision computed by ardrone onboard chip*/
	bool		vision_defined;

	/** static pointer to generic options */
	struct navdata_option  options[1];
};

struct navdata_demo
{
	/** Always 88776655 */
	uint32_t   header;
	/** Bit mask defined in SDK config.h */
	uint32_t   ardrone_state;
	/** Sequence number of the packet */
	uint32_t   sequence;
	/** True: vision computed by ardrone onboard chip */
	bool	   vision_defined;

	/** Type of the packet: must be TAG_DEMO.*/
	uint16_t   tag;
	/** Size of the packet in bytes */
	uint16_t   size;
	/** Flying state (landed, flying, hovering, etc.). */
	uint32_t   ctrl_state;
	/** Battery voltage filtered (mV) */
	uint32_t   vbat_flying_percentage;
	/** UAV's pitch in milli-degrees */
	float32_t  theta;
	/** UAV's roll in milli-degrees */
	float32_t  phi;
	/** UAV's yaw in milli-degrees */
	float32_t  psi;
	/** UAV's altitude in millimeters */
	int32_t    altitude;
	/** UAV's estimated linear velocity in millimeters/?s */
	float32_t  vx;
	/** UAV's estimated linear velocity in millimeters/?s */
	float32_t  vy;
	/** UAV's estimated linear velocity in millimeters/?s */
	float32_t  vz;
	/** Deprecated on ARdrone2.0*/
	uint32_t   num_frames;
	/** Deprecated on ARdrone2.0*/
	float32_t  detection_camera_rot[9];
	/** Deprecated on ARdrone2.0*/
	float32_t  detection_camera_trans[3];
	/** Deprecated on ARdrone2.0*/
	uint32_t   detection_tag_index;
	/** Type of tag searched in detection */
	uint32_t   detection_camera_type;
	/** Deprecated on ARdrone2.0*/
	float32_t  drone_camera_rot[9];
	/** Deprecated on ARdrone2.0*/
	float32_t  drone_camera_trans[3];
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
