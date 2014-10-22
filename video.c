#include "video.h"


//adresses pour la com vidéo
struct sockaddr_in addr_drone, addr_client;
int sock_video;

//routine de réception des packets vidéo
pthread_t video_thread;
fd_set vid_fd_set;
struct timeval video_timeout = {4, 0};
static volatile int stopped = 1;

char tcp_buf[TCP_VIDEO_BUF_SIZE];

//cette fonction est un thread, ne pas l'appeler !
void* video_routine(void* args) {
	ssize_t pack_size = 0;
	while(!stopped) {
		//a-t-on qqchose à recevoir ?
		if (select(sock_video+1, &vid_fd_set, NULL, NULL, &video_timeout) < 0) {
			perror("Erreur select()");
			stopped = 1;
		}
		else if(FD_ISSET(sock_video, &vid_fd_set)) {
			//recevoir le paquet du drone
			pack_size = recv(sock_video, tcp_buf, TCP_VIDEO_BUF_SIZE, 0);
			if (pack_size < 0)
				perror("Erreur recv()");
			printf("Reçu %u octets de vidéo.\n", pack_size);
		}
		else {
			printf("Timeout : aucune donnée vidéo reçue.\n");
			stopped = 1;
		}
	}
	pthread_exit(NULL);
}



int jakopter_init_video(lua_State* L) {
/*
	addr_drone.sin_family      = AF_INET;
	addr_drone.sin_addr.s_addr = inet_addr(WIFI_ARDRONE_IP);
	addr_drone.sin_port        = htons(PORT_VIDEO);
	*/
	//initialiser le fdset
	FD_ZERO(&vid_fd_set);
	addr_client.sin_family      = AF_INET;
	addr_client.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_client.sin_port        = htons(PORT_VIDEO);

	sock_video = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock_video < 0) {
		fprintf(stderr, "Erreur, impossible d'établir le socket\n");
		lua_pushnumber(L, -1);
		return 1;
	}

	//bind du socket client pour le forcer sur le port choisi
	if(connect(sock_video, (struct sockaddr*)&addr_drone, sizeof(addr_drone)) < 0) {
		perror("Erreur vidéo connect()");
		lua_pushnumber(L, -1);
		return 1;
	}
	//ajouter le socket au set pour select
	FD_SET(sock_video, &vid_fd_set);

	stopped = 0;
	//démarrer la réception des packets vidéo
	if(pthread_create(&video_thread, NULL, video_routine, NULL) < 0) {
		perror("Erreur création thread");
		stopped = 1;
		lua_pushnumber(L, -1);
		return 1;
	}

	lua_pushnumber(L, 0);
	return 1;
}

int jakopter_stop_video(lua_State* L) {
	stopped = 1;
	close(sock_video);
	lua_pushnumber(L, pthread_join(video_thread, NULL));
}
