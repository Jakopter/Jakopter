#include "com_master.h"
#include <stdbool.h>
#include <stdio.h>

//jakopter_com_channel_t* master = NULL;

bool isInitialized = true;

jakopter_com_channel_t* master[NB_CHANNELS];

int nb_max_chan = NB_CHANNELS;

int jakopter_com_init_master(int nb_chan_max)
{
	/*
	if(!isInitialized) {
		size_t total_size = sizeof(int) + nb_chan_max * sizeof(jakopter_com_channel_t*);
	
		master = jakopter_com_create_channel(total_size);
		if(master == NULL)
			return -1;
	
		nb_max_chan = nb_chan_max;
		isInitialized = true;
		return 0;
	}
	return -1;
	*/
	return 0;
}

int jakopter_com_master_is_init()
{
	return isInitialized;
}

jakopter_com_channel_t* jakopter_com_add_channel(int id, size_t size)
{
	if(id >= nb_max_chan) {
		fprintf(stderr, "[com_channel] Can't add channel with out-of-bounds id : %d\n", id);
		return NULL;
	}
	jakopter_com_channel_t* new_chan = jakopter_com_create_channel(size);
	if(new_chan == NULL)
		return NULL;
		
	//size_t offset = sizeof(int) + id*sizeof(jakopter_com_channel_t*);
	//jakopter_com_write_buf(master, offset, &new_chan, sizeof(new_chan));
	master[id] = new_chan;
	return new_chan;
}

jakopter_com_channel_t* jakopter_com_get_channel(int id)
{
	if(!isInitialized)
		return NULL;

	//jakopter_com_channel_t* chan;
	//size_t offset = sizeof(int) + id*sizeof(jakopter_com_channel_t*);
	//jakopter_com_read_buf(master, offset, sizeof(chan), &chan);
	
	return master[id];
}

int jakopter_com_remove_channel(int id)
{
	//get the channel's pointer and free it
	jakopter_com_channel_t* chan = jakopter_com_get_channel(id);
	jakopter_com_destroy_channel(&chan);
	return 0;
}

void jakopter_com_destroy_master()
{
	/*
	jakopter_com_destroy_channel(&master);
	nb_max_chan = 0;
	isInitialized = false;
	*/
}

