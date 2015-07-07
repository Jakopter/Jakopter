#ifndef JAKOPTER_VISP_H
#define JAKOPTER_VISP_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "com_master.h"
#define SIZE_MESSAGE 256
#define VISP_COM_IN_SIZE 32
#define VISP_COM_OUT_SIZE 3*sizeof(float)+sizeof(int)+SIZE_MESSAGE
int visp_process(uint8_t* frame, int width, int height, int size);
int visp_init();
void visp_destroy();
#ifdef __cplusplus
}
#endif
#endif