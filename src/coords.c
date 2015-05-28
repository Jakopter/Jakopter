#include "coords.h"

jakopter_com_channel_t* coords_channel;

pthread_t coords_thread;
//Guard that stops any function if connection isn't initialized.
bool stopped_coords = true;
static pthread_mutex_t mutex_coords = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_stopped_coords = PTHREAD_MUTEX_INITIALIZER;

static int read_coords(float* x, float* y, float* z)
{
	int ret = 0;
	FILE *coords_in = NULL;
	pthread_mutex_lock(&mutex_coords);
	coords_in = fopen(COORDS_FILENAME, "r");
	if (coords_in) {
		char* str;
		float num [3];
		int i = 0;
		int read_cnt = 0;

		do {
			//use space delimiter
			read_cnt = fscanf(coords_in,"%m[.0-9] ",&str);
			num[i] = (float)atof(str);
			free(str);
			i++;
		} while (read_cnt > 0 && i < 3);

		if (read_cnt > 0) {
			ret = 1;
			*x = num[0];
			*y = num[1];
			*z = num[2];
		}

		fclose(coords_in);
	}
	pthread_mutex_unlock(&mutex_coords);
	return ret;
}


void* coords_routine(void* args)
{
	pthread_mutex_lock(&mutex_stopped_coords); // protect the stop variable
	while (!stopped_coords) {
		pthread_mutex_unlock(&mutex_stopped_coords);

		float x = 0.0;
		float y = 0.0;
		float z = 0.0;

		int ret = read_coords(&x, &y, &z);
		if (ret) {
			// write only when you have a new value
			jakopter_com_write_int(coords_channel, 0, (float) x);
			jakopter_com_write_int(coords_channel, 4, (float) y);
			jakopter_com_write_int(coords_channel, 8, (float) z);
		}
		// wait before doing it again
		usleep(COORDS_INTERVAL*1000);
		pthread_mutex_lock(&mutex_stopped_coords);
	}
	pthread_mutex_unlock(&mutex_stopped_coords);
	pthread_exit(NULL);
}

int coords_connect()
{
	if (!stopped_coords)
		return -1;
	pthread_mutex_lock(&mutex_stopped_coords);
	stopped_coords = false;
	pthread_mutex_unlock(&mutex_stopped_coords);

	printf("[coords] connecting coords input\n");

	// right now it is just 2 int
	coords_channel = jakopter_com_add_channel(CHANNEL_COORDS, 3*sizeof(float));
	jakopter_com_write_float(coords_channel, 0, 0.0);
	jakopter_com_write_float(coords_channel, 4, 0.0);
	jakopter_com_write_float(coords_channel, 8, 0.0);

	printf("[coords] channel created\n");

	if (pthread_create(&coords_thread, NULL, coords_routine, NULL) < 0) {
		perror("[~][coords] Can't create thread");
		return -1;
	}

	printf("[coords] thread created\n");
	return 0;
}
int coords_disconnect()
{
	pthread_mutex_lock(&mutex_stopped_coords);
	if (!stopped_coords) {
		stopped_coords = true;
		pthread_mutex_unlock(&mutex_stopped_coords);
		int ret = pthread_join(coords_thread, NULL);

		jakopter_com_remove_channel(CHANNEL_COORDS);

		return ret;
	}
	else {
		pthread_mutex_unlock(&mutex_stopped_coords);

		fprintf(stderr, "[~][coords] Communication already stopped\n");
		return -1;
	}
}