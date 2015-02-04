#include "com_channel.h"

int jakopter_com_init_master(int nb_chan_max);

/**
* \brief Look up the given id in the master channel and return the
*		corresponding channel.
*/
jakopter_com_channel_t* jakopter_com_add_channel(int id, size_t size);

jakopter_com_channel_t* jakopter_com_get_channel(int id);

int jakopter_com_remove_channel(int id);

void jakopter_com_destroy_master();
