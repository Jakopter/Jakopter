#ifndef JAKOPTER_VIDEO_H
#define JAKOPTER_VIDEO_H


#include "common.h"

#define VIDEO_TIMEOUT 4
#define BASE_VIDEO_BUF_SIZE 1024
#define PORT_VIDEO		5555

/**
* Simple structure to hold a decoded video frame.
* The frame is assumed to be in the YUV420p image format.
*/
typedef struct jakopter_video_frame_t {
	int w, h;
	size_t size;
	uint8_t* pixels;
} jakopter_video_frame_t;

/*
Lancer le thread qui reçoit des paquets vidéo sur le port 5555
*/
int jakopter_init_video();
/*
Fermer la connexion au port et arrêter le thread.
*/
int jakopter_stop_video();
/*
Ask the video thread to stop, but don't wait for it. Shouldn't be called by the user.
*/
int video_set_stopped();

#endif

