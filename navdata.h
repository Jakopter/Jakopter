#ifndef NAVDATA_H
#define NAVDATA_H

#include "common.h"
#include "lauxlib.h"
#include "lua.h"
#define PORT_NAVDATA	5554
#define NAVDATA_INTERVAL 	1/15 // interval in seconds

typedef struct _navdata_option_t {
	uint16_t  tag;
	uint16_t  size;
	uint8_t   data[];
} navdata_option_t;


typedef struct _navdata_t {
	uint32_t    header;			/*!< Always set to NAVDATA_HEADER */
	uint32_t    ardrone_state;    /*!< Bit mask built from def_ardrone_state_mask_t */
	uint32_t    sequence;         /*!< Sequence number, incremented for each sent packet */
	bool		  vision_defined;

	navdata_option_t  options[1];
} __attribute__ ((packed)) navdata_t;


typedef struct _matrix33_t
{
  float32_t m11;
  float32_t m12;
  float32_t m13;
  float32_t m21;
  float32_t m22;
  float32_t m23;
  float32_t m31;
  float32_t m32;
  float32_t m33;
} matrix33_t;

typedef struct _vector31_t {
  union {
    float32_t v[3];
    struct
    {
      float32_t x;
      float32_t y;
      float32_t z;
    };
  };
} vector31_t;

typedef struct _navdata_demo_t {
  uint16_t    tag;					  /*!< Navdata block ('option') identifier */
  uint16_t    size;					  /*!< set this to the size of this structure */

  uint32_t    ctrl_state;             /*!< Flying state (landed, flying, hovering, etc.) defined in CTRL_STATES enum. */
  uint32_t    vbat_flying_percentage; /*!< battery voltage filtered (mV) */

  float32_t   theta;                  /*!< UAV's pitch in milli-degrees */
  float32_t   phi;                    /*!< UAV's roll  in milli-degrees */
  float32_t   psi;                    /*!< UAV's yaw   in milli-degrees */

  int32_t     altitude;               /*!< UAV's altitude in centimeters */

  float32_t   vx;                     /*!< UAV's estimated linear velocity */
  float32_t   vy;                     /*!< UAV's estimated linear velocity */
  float32_t   vz;                     /*!< UAV's estimated linear velocity */

  uint32_t    num_frames;			  /*!< streamed frame index */ // Not used -> To integrate in video stage.

  matrix33_t  detection_camera_rot;   /*!<  Deprecated ! Don't use ! */
  vector31_t  detection_camera_trans; /*!<  Deprecated ! Don't use ! */
  uint32_t	  detection_tag_index;    /*!<  Deprecated ! Don't use ! */

  uint32_t	  detection_camera_type;  /*!<  Type of tag searched in detection */

  matrix33_t  drone_camera_rot;		  /*!<  Deprecated ! Don't use ! */
  vector31_t  drone_camera_trans;	  /*!<  Deprecated ! Don't use ! */
}__attribute__ ((packed)) navdata_demo_t;

int navdata_connect();
int navdata_disconnect();
int jakopter_is_flying();
int jakopter_height();

#endif
