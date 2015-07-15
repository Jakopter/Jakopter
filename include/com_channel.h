#ifndef JAKOPTER_COM_CHANNEL_H
#define JAKOPTER_COM_CHANNEL_H
/**
* Jakopter communication channel :
* simple atomic structure to allow easy inter-thread communication
* through a fixed-size memory buffer.
* Only one operation is allowed at a time on the channel.
*/
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
*		The structure should be freed with jakopter_destroy_com_channel
*		once you're done using it.
*		Returns NULL on failure.
*/
jakopter_com_channel_t* jakopter_com_create_channel(size_t size);

/**
* \brief Free the memory allocated for the given com channel structure.
* \param cc com channel to free. The pointer to the structure will be set to NULL on success.
*/
void jakopter_com_destroy_channel (jakopter_com_channel_t** cc);


/*******************************************************************
********Functions for writing data into the channel*****************
*The following functions copy a given value or arbitrary memory zone
*into the communication channel, at the specified position.
*They make sure no buffer overflow occurs.
********************************************************************/

void jakopter_com_write_int(jakopter_com_channel_t* cc, size_t offset, int value);

void jakopter_com_write_float(jakopter_com_channel_t* cc, size_t offset, float value);

void jakopter_com_write_char(jakopter_com_channel_t* cc, size_t offset, char value);

void jakopter_com_write_buf(jakopter_com_channel_t* cc, size_t offset, void* data, size_t size);


/*******************************************************************
********Functions for reading data from the channel*****************/

int jakopter_com_read_int(jakopter_com_channel_t* cc, size_t offset);

float jakopter_com_read_float(jakopter_com_channel_t* cc, size_t offset);

char jakopter_com_read_char(jakopter_com_channel_t* cc, size_t offset);

/**
* \brief Read an arbitrary sequence of bytes.
* \param cc com channel to read from.
* \param offset position of the data to be read in the channel's buffer.
* \param size number of bytes to read.
* \param dest buffer where the read data will be copied.
*		Must point to a valid memory zone of at least size bytes.
* \return dest.
*/
void* jakopter_com_read_buf(jakopter_com_channel_t* cc, size_t offset, size_t size, void* dest);


/**
* \brief Get the timestamp, in milliseconds, of the last write operation
*		performed on a given com channel.
* \param cc com channel to check.
*/
double jakopter_com_get_timestamp(jakopter_com_channel_t* cc);

#endif

