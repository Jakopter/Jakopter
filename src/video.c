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
//Set to 1 when we want to tell the video thread to stop.
static volatile int stopped = 1;
static pthread_mutex_t mutex_stopped = PTHREAD_MUTEX_INITIALIZER;
//Set to 1 when the thread isn't running and it's safe to call init_video.
//If stopped == 0, this is guaranteed to be 0 as well. The contrary isn't true.
static volatile int terminated = 1;
static pthread_mutex_t mutex_terminated = PTHREAD_MUTEX_INITIALIZER;

//the init function needs to be synchronized
static pthread_mutex_t mutex_init = PTHREAD_MUTEX_INITIALIZER;

//TPC_VIDEO_BUF_SIZE = base size (defined in video.h) +
//extra size needed for decoding (see video_decode.h). (is useless now that we use a frame parser)
uint8_t tcp_buf[TCP_VIDEO_BUF_SIZE];

//clean things that have been initiated/created by init_video and need manual cleaning.
static void video_clean();

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
			video_set_stopped();
		}
		else if(FD_ISSET(sock_video, &vid_fd_set)) {

			/*receive the video data from the drone. Only BASE_SIZE, since
			TCP_SIZE may be larger on purpose.*/
			pack_size = recv(sock_video, tcp_buf, BASE_VIDEO_BUF_SIZE, 0);
			if(pack_size == 0) {
				printf("Stream ended by server. Ending the video thread.\n");
				video_set_stopped();
			}
			else if(pack_size < 0)
				perror("Error recv()");
			else {
				//we actually got some data, send it for decoding !
				nb_img = video_decode_packet(tcp_buf, pack_size);
				if(nb_img < 0) {
					fprintf(stderr, "Error processing frame !\n");
					video_set_stopped();
				}
			}
		}
		else {
			printf("Video : data reception has timed out. Ending the video thread now.\n");
			video_set_stopped();
		}
		//reset the timeout and the FDSET entry
		video_timeout.tv_sec = VIDEO_TIMEOUT;
		FD_ZERO(&vid_fd_set);
		FD_SET(sock_video, &vid_fd_set);

		pthread_mutex_lock(&mutex_stopped);
	}
	pthread_mutex_unlock(&mutex_stopped);
	//there's no reason to keep stuff that's needed for our video thread once it's ended, so clean it now.
	video_clean();
	pthread_mutex_lock(&mutex_terminated);
	terminated = 1;
	printf("Video thread terminated.\n");
	pthread_mutex_unlock(&mutex_terminated);
	pthread_exit(NULL);
}



int jakopter_init_video() {
	pthread_mutex_lock(&mutex_init);
	//do not try to initialize the thread if it's already running !
	pthread_mutex_lock(&mutex_terminated);
	if(!terminated) {
		pthread_mutex_unlock(&mutex_terminated);
		fprintf(stderr, "Video thread already running.\n");
		pthread_mutex_unlock(&mutex_init);
		return -1;
	}
	pthread_mutex_unlock(&mutex_terminated);
	
	//make the thread detached so that it can close itself without us having
	//to join with it in order to free its resources.
	pthread_attr_t thread_attribs;
	pthread_attr_init(&thread_attribs);
	pthread_attr_setdetachstate(&thread_attribs, PTHREAD_CREATE_DETACHED);

	addr_drone_video.sin_family      = AF_INET;
	addr_drone_video.sin_addr.s_addr = inet_addr(WIFI_ARDRONE_IP);
	addr_drone_video.sin_port        = htons(PORT_VIDEO);
	
	//initialiser le fdset
	FD_ZERO(&vid_fd_set);
	
	/* NOT NEEDED NOW
	//initialize the video buffer's extremity to zero to prevent
	//possible errors during the decoding process by ffmpeg.
	//Do not reference FF_INPUT_BUFFER_PADDING_SIZE directly to keep tasks as separated as possible.
	memset(tcp_buf+BASE_VIDEO_BUF_SIZE, 0, TCP_VIDEO_BUF_SIZE-BASE_VIDEO_BUF_SIZE);
	*/
	
	//initialize the video decoder
	if(video_init_decoder() < 0) {
		fprintf(stderr, "Error initializing decoder, aborting.\n");
		pthread_attr_destroy(&thread_attribs);
		pthread_mutex_unlock(&mutex_init);
		return -1;
	}

	sock_video = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_video < 0) {
		fprintf(stderr, "Error : couldn't bind TCP socket.\n");
		video_stop_decoder();
		pthread_attr_destroy(&thread_attribs);
		pthread_mutex_unlock(&mutex_init);
		return -1;
	}

	//bind du socket client pour le forcer sur le port choisi
	if(connect(sock_video, (struct sockaddr*)&addr_drone_video, sizeof(addr_drone_video)) < 0) {
		perror("Error connecting to video stream");
		video_clean();
		pthread_attr_destroy(&thread_attribs);
		pthread_mutex_unlock(&mutex_init);
		return -1;
	}
	//ajouter le socket au set pour select
	FD_SET(sock_video, &vid_fd_set);
	
	stopped = 0;
	terminated = 0;
	//démarrer la réception des packets vidéo
	if(pthread_create(&video_thread, &thread_attribs, video_routine, NULL) < 0) {
		perror("Error creating the main video thread");
		stopped = 1;
		terminated = 1;
		video_clean();
		pthread_attr_destroy(&thread_attribs);
		pthread_mutex_unlock(&mutex_init);
		return -1;
	}
	pthread_attr_destroy(&thread_attribs);
	//now that we're done initializing, release the lock.
	pthread_mutex_unlock(&mutex_init);
	return 0;
}


void video_clean() {
	if(close(sock_video) < 0)
		perror("Error stopping video connection");
	video_stop_decoder();
}

/*Ask the video thread to stop without joining with it.
Useful for stopping it from the inside.
@return 0 if stopped was 0, 1 if it wasn't.*/
int video_set_stopped() {
	pthread_mutex_lock(&mutex_init);
	pthread_mutex_lock(&mutex_stopped);
	if(!stopped) {
		stopped = 1;
		pthread_mutex_unlock(&mutex_stopped);
		pthread_mutex_unlock(&mutex_init);
		return 0;
	}
	else {
		pthread_mutex_unlock(&mutex_stopped);
		pthread_mutex_unlock(&mutex_init);
		return 1;
	}
}

/*
End the video thread and clean the required structures
*/
int jakopter_stop_video() {

	if(video_set_stopped()) {
		fprintf(stderr, "Video thread is already shut down.\n");
		return -1;
	}
	else
		return 0;
}

