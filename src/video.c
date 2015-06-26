#include "video.h"
#include "video_queue.h"
#include "video_decode.h"
#include "video_display.h"
#include "visp.h"


/*addresses for video communication*/
struct sockaddr_in addr_drone_video, addr_client_video;
int sock_video;

/*video packet reception, and video processing routines*/
pthread_t video_thread, processing_thread;
/*FD set used for the video socket*/
fd_set vid_fd_set;
struct timeval video_timeout = {VIDEO_TIMEOUT, 0};

/** Video processing API implementation */
static const struct jakopter_frame_processing frame_process = {
	.callback = video_display_process,
	.init     = video_display_init,
	.clean    = video_display_destroy
	// .callback = visp_process,
	// .init     = visp_init,
	// .clean    = visp_destroy
};

/** Drawing API implementation */
static const struct jakopter_drawing draw_implementation = {
	.draw_icon   = display_draw_icon,
	.draw_text   = display_draw_text,
	.remove      = display_graphic_remove,
	.resize      = display_graphic_resize,
	.move        = display_graphic_move
	// .draw_icon   = NULL,
	// .draw_text   = NULL,
	// .remove      = NULL,
	// .resize      = NULL,
	// .move        = NULL
};

/*Set to 1 when we want to tell the video thread to stop.*/
static volatile int stopped = 1;
static pthread_mutex_t mutex_stopped = PTHREAD_MUTEX_INITIALIZER;
/*Set to 1 when the thread isn't running and it's safe to call init_video.*/
/*If stopped == 0, this is guaranteed to be 0 as well. The contrary isn't true.*/
static volatile int terminated = 1;
static pthread_mutex_t mutex_terminated = PTHREAD_MUTEX_INITIALIZER;


/*clean things that have been initiated/created by init_video and need manual cleaning.*/
static void video_clean();
static int video_join_thread();


void* video_routine(void* args)
{
	/*TCP segment of encoded video received from the drone*/
	static uint8_t tcp_buf[TCP_VIDEO_BUF_SIZE];
	/*size of this segment in bytes*/
	ssize_t pack_size = 0;
	/*set to 1 by the decoding function if a decoded video frame is available*/
	int got_frame = 0;
	/*structure that will hold our latest decoded video frame*/
	jakopter_video_frame_t decoded_frame;

	pthread_mutex_lock(&mutex_stopped);
	while (!stopped) {
		pthread_mutex_unlock(&mutex_stopped);
		//Wait for the drone to send data on the video socket
		if (select(sock_video+1, &vid_fd_set, NULL, NULL, &video_timeout) < 0) {
			perror("[~][video] Cannot select()");
			video_set_stopped();
		}
		else if (FD_ISSET(sock_video, &vid_fd_set)) {
			/*receive the video data from the drone. Only BASE_SIZE, since
			TCP_SIZE may be larger on purpose.*/
			pack_size = recv(sock_video, tcp_buf, BASE_VIDEO_BUF_SIZE, 0);
			if (pack_size == 0) {
				printf("[*][video] Stream ended by server. Ending the video thread.\n");
				video_set_stopped();
			}
			else if (pack_size < 0)
				perror("[~][video] Cannot recv()");
			else {
				//we actually got some data, send it for decoding !
				got_frame = video_decode_packet(tcp_buf, pack_size, &decoded_frame);
				if (got_frame < 0) {
					fprintf(stderr, "[~][video] Cannot decoding video\n");
					video_set_stopped();
				}
				//if we have a complete decoded frame, push it onto the queue for decoding
				else if (got_frame == 1)
					video_queue_push_frame(&decoded_frame);
			}
		}
		else {
			printf("[*][video] Data reception has timed out. Ending the video thread now.\n");
			video_set_stopped();
		}
		//reset the timeout and the FDSET entry
		video_timeout.tv_sec = VIDEO_TIMEOUT;
		FD_ZERO(&vid_fd_set);
		FD_SET(sock_video, &vid_fd_set);

		pthread_mutex_lock(&mutex_stopped);
	}
	pthread_mutex_unlock(&mutex_stopped);
	/*push an empty frame on the queue so the processing
	thread knows it has to stop*/
	video_queue_push_frame(&VIDEO_QUEUE_END);
	pthread_join(processing_thread, NULL);
	//there's no reason to keep stuff that's needed for our video thread once it's ended, so clean it now.
	video_clean();
	pthread_exit(NULL);
}

void* processing_routine(void* args)
{
	//decoded video frame that will be pulled from the queue
	jakopter_video_frame_t frame;
	//wait for frames to be decoded, and then process them.
	pthread_mutex_lock(&mutex_stopped);
	while (!stopped) {
		pthread_mutex_unlock(&mutex_stopped);
		if (video_queue_pull_frame(&frame) < 0) {
			fprintf(stderr, "[~][Video Processing] Cannot retrieving frame\n");
			video_set_stopped();
		}
		//a 0-sized frame means we're about to quit.
		else if (frame.size != 0) {
			if (frame_process.callback(frame.pixels, frame.w, frame.h, frame.size) < 0) {
				fprintf(stderr, "[~][Video Processing] Cannot processing frame\n");
				video_set_stopped();
			}
		}
		pthread_mutex_lock(&mutex_stopped);
	}
	pthread_mutex_unlock(&mutex_stopped);
	//free the resources of the processing module
	if (frame_process.clean != NULL)
		frame_process.clean();
	pthread_exit(NULL);
}

int jakopter_init_video(const char* drone_ip)
{
	//do not try to initialize the thread if it's already running !
	pthread_mutex_lock(&mutex_stopped);
	if (!stopped) {
		fprintf(stderr, "[~][video] Video thread already running.\n");
		pthread_mutex_unlock(&mutex_stopped);
		return -1;
	}
	//make sure the thread is terminated
	video_join_thread();

	addr_drone_video.sin_family      = AF_INET;
	if (drone_ip == NULL)
		addr_drone_video.sin_addr.s_addr = inet_addr(WIFI_ARDRONE_IP);
	else
		addr_drone_video.sin_addr.s_addr = inet_addr(drone_ip);
	addr_drone_video.sin_port        = htons(PORT_VIDEO);

	if (addr_drone_video.sin_addr.s_addr == INADDR_NONE) {
		perror("[~] The drone adress is invalid \n");
		return -1;
	}

	//initialize the fdset
	FD_ZERO(&vid_fd_set);

	//initialize the video decoder
	if (video_init_decoder() < 0) {
		fprintf(stderr, "[~][video] cannot initialize decoder, aborting.\n");
		pthread_mutex_unlock(&mutex_stopped);
		return -1;
	}

	sock_video = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_video < 0) {
		fprintf(stderr, "[~][video] couldn't bind TCP socket.\n");
		video_stop_decoder();
		pthread_mutex_unlock(&mutex_stopped);
		return -1;
	}

	//bind the client socket to force it on the drone's port
	if (connect(sock_video, (struct sockaddr*)&addr_drone_video, sizeof(addr_drone_video)) < 0) {
		perror("[~][video] cannot connect to video stream");
		video_clean();
		pthread_mutex_unlock(&mutex_stopped);
		return -1;
	}
	//add the socket to the set, for use with select()
	FD_SET(sock_video, &vid_fd_set);

	//initialize the queue structure that handles decoding->processing data passing
	video_queue_init();
	//start the threads responsible for video processing and reception
	if (pthread_create(&processing_thread, NULL, processing_routine, NULL) < 0) {
		perror("[~][video] cannot create the video processing thread");
		video_clean();
		pthread_mutex_unlock(&mutex_stopped);
		return -1;
	}
	//make the reception thread detached so that it can close itself without us having
	//to join with it in order to free its resources.
	/*pthread_attr_t thread_attribs;
	pthread_attr_init(&thread_attribs);
	pthread_attr_setdetachstate(&thread_attribs, PTHREAD_CREATE_DETACHED);*/
	if (pthread_create(&video_thread, NULL, video_routine, NULL) < 0) {
		perror("[~][video] cannot create the main video thread");
		video_clean();
		//pthread_attr_destroy(&thread_attribs);
		pthread_mutex_unlock(&mutex_stopped);
		return -1;
	}
	//pthread_attr_destroy(&thread_attribs);
	//Initialization went OK -> set the guard variables so that the threads can start.
	if (frame_process.init != NULL)
		frame_process.init();
	stopped = 0;
	terminated = 0;
	pthread_mutex_unlock(&mutex_stopped);
	return 0;
}

int jakopter_draw_icon(const char *p, int x, int y, int w, int h)
{
	if(draw_implementation.draw_icon == NULL)
		return -1;
	return draw_implementation.draw_icon(p, x, y, w, h);
}

int jakopter_draw_text(const char *s, int x, int y)
{
	if(draw_implementation.draw_text == NULL)
		return -1;
	return draw_implementation.draw_text(s, x, y);
}

void jakopter_draw_remove(int id)
{
	if(draw_implementation.remove == NULL)
		return;
	draw_implementation.remove(id);
}

void jakopter_draw_resize(int id, int width, int height)
{
	if (draw_implementation.resize != NULL)
		draw_implementation.resize(id, width, height);
}

void jakopter_draw_move(int id, int x, int y)
{
	if (draw_implementation.move != NULL)
		draw_implementation.move(id, x, y);
}

void video_clean()
{
	if (close(sock_video) < 0)
		perror("[~][video] cannot stop video connection");
	video_stop_decoder();
	video_queue_free();
}

int video_set_stopped()
{
	pthread_mutex_lock(&mutex_stopped);
	if (!stopped) {
		stopped = 1;
		pthread_mutex_unlock(&mutex_stopped);
		return 0;
	}
	else {
		pthread_mutex_unlock(&mutex_stopped);
		return 1;
	}
}

int jakopter_stop_video()
{
	video_set_stopped();
	int exit_status = video_join_thread();
	if (exit_status == 1)
		fprintf(stderr, "[*][video] Video thread is already shut down.\n");
	else if (exit_status < 0)
		return -1;

	return 0;
}

/**
  * \brief Join with the video thread (don't ask it to stop though).
  * If it's already terminated, do nothing.
  * \return the thread's termination status when entering the function.
  *		(0 if it was running, 1 if it wasn't), -1 if joining fails.
  */
int video_join_thread()
{
	pthread_mutex_lock(&mutex_terminated);
	//remember the current state of the thread to return it at the end.
	int prev_state = terminated;
	if (!terminated) {
		if (pthread_join(video_thread, NULL) < 0) {
			perror("[~][video] cannot join with the video thread");
			pthread_mutex_unlock(&mutex_terminated);
			return -1;
		}
		terminated = 1;
	}
	pthread_mutex_unlock(&mutex_terminated);
	return prev_state;
}

