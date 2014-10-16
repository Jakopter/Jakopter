#ifndef NAVDATA_H
#define NAVDATA_H

#include <stdint.h>
#include <stdbool.h>

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

int navdata_connect(lua_State* L);
int navdata_disconnect(lua_State* L);

#endif
