#ifndef JAKOPTER_COM_MASTER_H
#define JAKOPTER_COM_MASTER_H
#include "com_channel.h"

/**
* List of reserved channels.
* If you need a channel, you should add it here.
* Valid channels id range from CHANNEL_MASTER up to NB_CHANNELS, both excluded.
*/
enum jakopter_channels {
	CHANNEL_MASTER,
	CHANNEL_NAVDATA,
	CHANNEL_DISPLAY,
	CHANNEL_LEAPMOTION,
	CHANNEL_USERINPUT,
	CHANNEL_COORDS,
	CHANNEL_CALLBACK,
	NB_CHANNELS
};

/**
* \brief Create a channel and index it in the master list.
* \param id identificator of the channel to be created.
*		Should be between 1 and NB_CHANNELS (excluded).
*		Shouldn't be the id of a currently active channel.
* \param size Size of the new channel in bytes.
* \returns a pointer to the new channel, that can be used
*		with read/write functions.
*		NULL if there was an error.
*/
jakopter_com_channel_t* jakopter_com_add_channel(int id, size_t size);

/**
* \brief Look up the given id in the master channel and return the
*		corresponding channel.
* \param id identificator of the channel.
*			It should belong to the jakopter_channels enum.
* \returns a pointer to the requested channel, ready to be used
*			with read/write com_channel functions.
*			NULL if the requested channel doesn't exist.
*/
jakopter_com_channel_t* jakopter_com_get_channel(int id);

/**
* \brief Remove the requested channel from the master index,
*		and free its resources.
* \param id identificator of the channel to be deleted.
* \returns 0 on success, -1 if the channel doesn't exist.
*/
int jakopter_com_remove_channel(int id);

#endif
