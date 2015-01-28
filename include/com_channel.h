/**
* Jakopter communication channel :
* simple atomic structure to allow easy inter-thread communication
* through a fixed-size memory buffer.
* Only one operation is allowed at a time on the channel.
*/
#ifndef JAKOPTER_COM_CHANNEL_H
#define JAKOPTER_COM_CHANNEL_H
#include <stddef.h>
#include <stdint.h>
/**
* \brief The structure representing the communication channel.
*		It should only be manipulated through the functions
*		exposed in the com_channel interface.
*/
typedef struct jakopter_com_channel_t jakopter_com_channel_t;

/**
* \brief Initialize a com_channel structure, with the given buffer size.
* \param size Size of the communication buffer in bytes.
* \returns jakopter_com_channel_t structure, ready to be used.
*		The strucutre should be freed with jakopter_destroy_com_channel
*		once you're done using it.
*		Returns NULL on failure.
*/
jakopter_com_channel_t* jakopter_init_com_channel(size_t size);

/**
* \brief Free the memory allocated for the given com channel structure.
* \param cc com channel to free. The pointer to the structure will be set to NULL on success.
*/
void jakopter_destroy_com_channel (jakopter_com_channel_t** cc);


/*******************************************************************
********Functions for writing data into the channel*****************
*The following functions copy a given value or arbitrary memory zone
*into the communication channel, at the specified position.
*They make sure no buffer overflow occurs.
********************************************************************/

void jakopter_write_int(jakopter_com_channel_t* cc, size_t offset, int value);

void jakopter_write_char(jakopter_com_channel_t* cc, size_t offset, char value);

void jakopter_write_buf(jakopter_com_channel_t* cc, size_t offset, void* data, size_t size);


/*******************************************************************
********Functions for reading data from the channel*****************/

int jakopter_read_int(jakopter_com_channel_t* cc, size_t offset);

char jakopter_read_char(jakopter_com_channel_t* cc, size_t offset);

//The returned pointer has to be freed by the caller.
void* jakopter_read_buf(jakopter_com_channel_t* cc, size_t offset, size_t size);


/**
* \brief Get the timestamp, in milliseconds, of the last write operation
*		performed on a given com channel.
* \param cc com channel to check.
*/
double jakopter_get_timestamp(jakopter_com_channel_t* cc);

#endif

