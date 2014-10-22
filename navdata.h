#ifndef NAVDATA_H
#define NAVDATA_H

#include "common.h"
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

int navdata_connect();
int navdata_disconnect();

#endif
