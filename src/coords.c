/* Jakopter
 * Copyright © 2015 ALF@INRIA
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/un.h>

#include "coords.h"

/* The string which contains the last coords received.*/
static char log_coords[TSTAMP_LEN+3+COORDS_BUF_SIZE];
struct sockaddr_un addr_server_coords;
int sock_coords;

jakopter_com_channel_t* coords_channel;
pthread_t coords_thread;
/*Guard that stops any function if connection isn't initialized.*/
static bool recv_ready = false;
static bool stopped_coords = true;
static pthread_mutex_t mutex_stopped_coords = PTHREAD_MUTEX_INITIALIZER;
/* Race condition between requesting timestamp and record of timestamp*/
static pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;

static int read_coords(float num[COORDS_NREADS])
{
	int read_cnt = 0;
	char buf [COORDS_BUF_SIZE];
	socklen_t len = sizeof(addr_server_coords);

	read_cnt = recvfrom(sock_coords, &buf, sizeof(buf), 0, (struct sockaddr*)&addr_server_coords, &len);

	if (read_cnt < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		return 0;
	}
	else if (read_cnt < 0) {
		perror("[~][coords]recvfrom() failed");
		return -1;
	}
	/* Log */
	pthread_mutex_lock(&mutex_log);
	memset(log_coords, '\0', TSTAMP_LEN+3+COORDS_BUF_SIZE);
	struct timespec ts = {0,0};
	clock_gettime(CLOCK_REALTIME, &ts);
	snprintf(log_coords, TSTAMP_LEN+3, "%lu.%lu:c ", ts.tv_sec, ts.tv_nsec);
	strncat(log_coords, buf, COORDS_BUF_SIZE);
	pthread_mutex_unlock(&mutex_log);
	/* Parsing */
	int i = 0;
	do {
		// use space delimiter
		char* str = NULL;
		if (i == 0)
			str = strtok(buf, " ");
		else
			str = strtok(NULL, " ");
		if (str == NULL)
			break;


		num[i] = strtof(str, NULL);
		if (errno != 0) {
			perror("[~][coords]strtof() failed");
			break;
		}
		i++;
	} while (i < COORDS_NREADS);

	return read_cnt > 0;
}


void* coords_routine(void* args)
{
	float num[COORDS_NREADS];
	pthread_mutex_lock(&mutex_stopped_coords);
	while (!stopped_coords) {
		pthread_mutex_unlock(&mutex_stopped_coords);

		// write only when you have a new value
		if (read_coords(num)) {
			for (int i = 0; i < COORDS_NREADS; i++)
				jakopter_com_write_float(coords_channel, sizeof(float)*i, num[i]);
		}
		recv_ready = true;
		// wait before doing it again
		usleep(COORDS_INTERVAL*1000);
		pthread_mutex_lock(&mutex_stopped_coords);
	}
	pthread_mutex_unlock(&mutex_stopped_coords);
	pthread_exit(NULL);
}

int jakopter_init_coords()
{
	if (!stopped_coords)
		return -1;

	printf("[coords] connecting coords input\n");

	memset(&addr_server_coords, '\0', sizeof(struct sockaddr_un));
	addr_server_coords.sun_family = AF_UNIX;
	strncpy(addr_server_coords.sun_path, COORDS_FILENAME, sizeof(addr_server_coords.sun_path)-1);

	sock_coords = socket(AF_UNIX, SOCK_DGRAM, 0);

	if (sock_coords < 0) {
		perror("[~][coords] Can't create the socket");
		return -1;
	}

	if (bind (sock_coords, (struct sockaddr*)&addr_server_coords, sizeof(struct sockaddr_un)) < 0) {
		perror("[~][coords] Can't bind the socket");
		close(sock_coords);
		unlink(COORDS_FILENAME);
		return -1;
	}

	coords_channel = jakopter_com_add_channel(CHANNEL_COORDS, COORDS_NREADS*sizeof(float));

	for (int i = 0; i < COORDS_NREADS; i++)
		jakopter_com_write_float(coords_channel, sizeof(float)*i, 0.0);


	printf("[coords] channel created\n");
	pthread_mutex_lock(&mutex_stopped_coords);
	stopped_coords = false;
	pthread_mutex_unlock(&mutex_stopped_coords);

	if (pthread_create(&coords_thread, NULL, coords_routine, NULL) < 0) {
		perror("[~][coords] Can't create thread");
		close(sock_coords);
		unlink(COORDS_FILENAME);
		return -1;
	}

	printf("[coords] thread created\n");


	int i = 0;
	while (!recv_ready && i < COORDS_TIMEOUT) {
		usleep(500);
		i++;
	}

	return -(i >= COORDS_TIMEOUT);
}
int jakopter_stop_coords()
{
	pthread_mutex_lock(&mutex_stopped_coords);
	if (!stopped_coords) {
		stopped_coords = true;
		pthread_mutex_unlock(&mutex_stopped_coords);
		int ret = pthread_join(coords_thread, NULL);

		jakopter_com_remove_channel(CHANNEL_COORDS);

		close(sock_coords);
		printf("[*] Unlink\n");
		unlink(COORDS_FILENAME);

		return ret;
	}
	else {
		pthread_mutex_unlock(&mutex_stopped_coords);

		fprintf(stderr, "[~][coords] Communication already stopped\n");
		return -1;
	}
}

const char* jakopter_log_coords()
{
	static char ret[TSTAMP_LEN+COORDS_BUF_SIZE];
	if (!stopped_coords) {
		pthread_mutex_lock(&mutex_log);
		strncpy(ret, log_coords, TSTAMP_LEN+COORDS_BUF_SIZE);
		pthread_mutex_unlock(&mutex_log);
		return ret;
	}
	else
		return NULL;
}
