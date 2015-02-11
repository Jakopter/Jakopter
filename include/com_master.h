#ifndef JAKOPTER_COM_MASTER_H
#define JAKOPTER_COM_MASTER_H
#include "com_channel.h"

enum jakopter_channels {
	CHANNEL_MASTER,
	CHANNEL_NAVDATA,
	CHANNEL_DISPLAY,
	NB_CHANNELS
};

int jakopter_com_init_master(int nb_chan_max);

int jakopter_com_master_is_init();

/**
* \brief Look up the given id in the master channel and return the
*		corresponding channel.
*/
jakopter_com_channel_t* jakopter_com_add_channel(int id, size_t size);

jakopter_com_channel_t* jakopter_com_get_channel(int id);

int jakopter_com_remove_channel(int id);

void jakopter_com_destroy_master();

#endif
