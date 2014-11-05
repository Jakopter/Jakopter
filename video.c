#include "video.h"
#include "video_decode.h"

//adresses pour la com vidéo
struct sockaddr_in addr_drone_video, addr_client_video;
int sock_video;

//routine de réception des packets vidéo
pthread_t video_thread;
//FD set used for the video socket
fd_set vid_fd_set;
struct timeval video_timeout = {VIDEO_TIMEOUT, 0};
static volatile int stopped = 1;
static pthread_mutex_t mutex_stopped = PTHREAD_MUTEX_INITIALIZER;

//TPC_VIDEO_BUF_SIZE = base size (defined in video.h) +
//extra size needed for decoding (see video_decode.h).
uint8_t tcp_buf[TCP_VIDEO_BUF_SIZE];

//cette fonction est un thread, ne pas l'appeler !
void* video_routine(void* args) {
	ssize_t pack_size = 0;
	int nb_img = 0;
	pthread_mutex_lock(&mutex_stopped);
	while(!stopped) {
		pthread_mutex_unlock(&mutex_stopped);
		//Wait for the drone to send data on the video socket
		if (select(sock_video+1, &vid_fd_set, NULL, NULL, &video_timeout) < 0) {
			perror("Error select()");
			stopped = 1;
		}
		else if(FD_ISSET(sock_video, &vid_fd_set)) {
			//receive the video data from the drone. Remember to leave some room
			//at the end of the buffer.
			pack_size = recv(sock_video, tcp_buf, BASE_VIDEO_BUF_SIZE, 0);
			if (pack_size < 0)
				perror("Error recv()");

			//printf("Reçu %zd octets de vidéo.\n", pack_size);
			nb_img = video_decode_packet(tcp_buf, pack_size);
			printf("Decoded %d frames(s)\n", nb_img);
		}
		else {
			printf("Video : data reception has timed out. Ending the video thread now.\n");
			//printf("Timeout : aucune donnée vidéo reçue. Nouvel essai.\n");
			stopped = 1;
		}
		//reset the timeout
		video_timeout.tv_sec = VIDEO_TIMEOUT;
		pthread_mutex_lock(&mutex_stopped);
	}
	pthread_mutex_unlock(&mutex_stopped);
	pthread_exit(NULL);
}



int jakopter_init_video(lua_State* L) {

	addr_drone_video.sin_family      = AF_INET;
	addr_drone_video.sin_addr.s_addr = inet_addr(WIFI_ARDRONE_IP);
	addr_drone_video.sin_port        = htons(PORT_VIDEO);
	
	//initialiser le fdset
	FD_ZERO(&vid_fd_set);

	sock_video = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_video < 0) {
		fprintf(stderr, "Error : couldn't bind TCP socket.\n");
		lua_pushnumber(L, -1);
		return 1;
	}

	//bind du socket client pour le forcer sur le port choisi
	if(connect(sock_video, (struct sockaddr*)&addr_drone_video, sizeof(addr_drone_video)) < 0) {
		perror("Error connecting to video stream");
		close(sock_video);
		lua_pushnumber(L, -1);
		return 1;
	}
	//ajouter le socket au set pour select
	FD_SET(sock_video, &vid_fd_set);
	
	//initialize the video buffer's extremity to zero to prevent
	//possible errors during the decoding process by ffmpeg.
	//Do not reference FF_INPUT_BUFFER_PADDING_SIZE directly to keep tasks as separated as possible.
	memset(tcp_buf+BASE_VIDEO_BUF_SIZE, 0, TCP_VIDEO_BUF_SIZE-BASE_VIDEO_BUF_SIZE);

	//initialize the video decoder
	if(video_init_decoder() < 0) {
		fprintf(stderr, "Error initializing decoder, aborting.\n");
		close(sock_video);
		lua_pushnumber(L, -1);
		return 1;
	}
	
	pthread_mutex_lock(&mutex_stopped);
	stopped = 0;
	pthread_mutex_unlock(&mutex_stopped);
	//démarrer la réception des packets vidéo
	if(pthread_create(&video_thread, NULL, video_routine, NULL) < 0) {
		perror("Error creating the main video thread");
		stopped = 1;
		close(sock_video);
		video_stop_decoder();
		lua_pushnumber(L, -1);
		return 1;
	}

	lua_pushnumber(L, 0);
	return 1;
}

int jakopter_stop_video(lua_State* L) {

	pthread_mutex_lock(&mutex_stopped);
	if(!stopped) {
		pthread_mutex_unlock(&mutex_stopped);
		close(sock_video);
		video_stop_decoder();
		lua_pushnumber(L, pthread_join(video_thread, NULL));
	}
	else {
		pthread_mutex_unlock(&mutex_stopped);
		fprintf(stderr, "Video thread is already shut down.\n");
		lua_pushnumber(L, -1);
	}
	return 1;
}
