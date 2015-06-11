#include "user_input.h"
#include <fcntl.h>

jakopter_com_channel_t* user_input_channel;

//static int fdpipe;

pthread_t user_input_thread;
//Guard that stops any function if connection isn't initialized.
bool stopped_user_input = true;
static pthread_mutex_t mutex_user_input = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_stopped_user_input = PTHREAD_MUTEX_INITIALIZER;

int read_cmd()
{
	int ret = -1;
	char c;
	FILE *keyboard_cmd = NULL;
	pthread_mutex_lock(&mutex_user_input);
	keyboard_cmd = fopen(CMDFILENAME,"r");
	if (keyboard_cmd) {
		fscanf(keyboard_cmd,"%c",&c);
		ret = (int) c;
		fclose(keyboard_cmd);
	}
	// char* buf = NULL;
	// if (fdpipe) {
	// 	read(fdpipe, &buf, 256);
	// 	if(buf != NULL)
	// 		ret = atoi(buf);
	// }

	pthread_mutex_unlock(&mutex_user_input);
	return ret;
}

int user_input_init()
{
	return 0;
}

/*user_input_thread function*/
void* user_input_routine(void* args)
{
	int keyboard_key = 0;
	int param2 = 0;
	int last_key = 0;
	int pparam2 = 0;

	pthread_mutex_lock(&mutex_stopped_user_input); // protect the stop variable
	while (!stopped_user_input) {
		pthread_mutex_unlock(&mutex_stopped_user_input);
		// .... do something here
		keyboard_key = read_cmd();

		if (keyboard_key < 0)
			pthread_exit(NULL);

		param2 = 0; // not used yet

		if ((keyboard_key != last_key) || (pparam2 != param2)) {
			// write only when you have a new value
			jakopter_com_write_int(user_input_channel, 0, (int) keyboard_key);
			jakopter_com_write_int(user_input_channel, 4, (int) param2);
			last_key = keyboard_key;
			pparam2 = param2;
		}

		// wait before doing it again
		usleep(USERINPUT_INTERVAL*1000);
		pthread_mutex_lock(&mutex_stopped_user_input);
	}
	pthread_mutex_unlock(&mutex_stopped_user_input);
	pthread_exit(NULL);
}

int user_input_connect()
{
	if (!stopped_user_input)
		return -1;

	printf("[user_input] connecting user input\n");
	pthread_mutex_lock(&mutex_stopped_user_input);
	stopped_user_input = false;
	pthread_mutex_unlock(&mutex_stopped_user_input);

	// right now it is just 2 int
	user_input_channel = jakopter_com_add_channel(CHANNEL_USERINPUT, 2*sizeof(int));
	jakopter_com_write_int(user_input_channel, 0, 0);
	jakopter_com_write_int(user_input_channel, 4, 0);

	// fdpipe = open(CMDFILENAME, O_RDONLY);

	// if (fdpipe < 0) {
	// 	perror("[~][user_input] Can't open named pipe");
	// 	return -1;
	// }

	printf("[user_input] channel created\n");

	if (pthread_create(&user_input_thread, NULL, user_input_routine, NULL) < 0) {
		perror("[~][user_input] Can't create thread");
		return -1;
	}

	printf("[user_input] thread created\n");
	return 0;
}

int user_input_disconnect()
{
	pthread_mutex_lock(&mutex_stopped_user_input);
	if (!stopped_user_input) {
		stopped_user_input = true;
		pthread_mutex_unlock(&mutex_stopped_user_input);
		int ret = pthread_join(user_input_thread, NULL);

		//close(fdpipe);
		jakopter_com_remove_channel(CHANNEL_USERINPUT);

		return ret;
	}
	else {
		pthread_mutex_unlock(&mutex_stopped_user_input);

		fprintf(stderr, "[~][user_input] Communication already stopped\n");
		return -1;
	}
}

