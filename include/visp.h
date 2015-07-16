#ifndef JAKOPTER_VISP_H
#define JAKOPTER_VISP_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "com_master.h"
#include "utils.h"
#define SIZE_MESSAGE 256
#define VISP_COM_IN_SIZE 32
#define VISP_COM_OUT_SIZE 3*sizeof(float)+sizeof(int)+SIZE_MESSAGE
#define VIDEO_TITLE "Jakopter - Video stream"
enum processes {
	FACE,
	QRCODE,
	BLOB
};
/** \brief Use a process on the current frame and display it in a window.
  * The default process is face detection.
  * \param frame the last frame received from the drone in YUV420p
  * \param width in pixels
  * \param height in pixels
  * \param size in bytes
  */
int visp_process(uint8_t* frame, int width, int height, int size);
/** \brief Set a process to use on the frame
  * \param id correspond to a value in enum processes
  */
void visp_set_process(int id);
/** \brief Init com_channels */
int visp_init();
/** \brief Clean objects and com_channels*/
void visp_destroy();
#ifdef __cplusplus
}
#endif
#endif