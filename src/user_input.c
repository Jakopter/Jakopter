/* Jakopter
 * Copyright © 2014 - 2015 Thibaud Hulin, Hector Labanca, Jérémy Yziquel
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
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/un.h>

#include "user_input.h"

jakopter_com_channel_t* user_input_channel;

pthread_t user_input_thread;
static bool recv_ready = false;
/* Guard that stops any function if connection isn't initialized.*/
bool stopped_user_input = true;

struct sockaddr_un addr_user_input;
int sock_user_input;

int read_cmd()
{
	int ret = -1;
	char c;
	FILE *keyboard_cmd = NULL;
	keyboard_cmd = fopen(USERINPUT_FILENAME,"r");
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

	return ret;
}

/*user_input_thread function*/
void* user_input_routine(void* args)
{
	int keyboard_key = 0;
	int param2 = 0;
	int last_key = 0;
	int pparam2 = 0;


	while (!stopped_user_input) {
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
	}
	pthread_exit(NULL);
}

int user_input_connect()
{
	if (!stopped_user_input)
		return -1;

	stopped_user_input = false;
	/*
	memset(&addr_user_input, '\0', sizeof(struct sockaddr_un));
	addr_user_input.sun_family = AF_UNIX;
	strncpy(addr_user_input.sun_path, USERINPUT_FILENAME, sizeof(addr_user_input.sun_path)-1);

	sock_user_input = socket(AF_UNIX, SOCK_DGRAM, 0);

	if (sock_user_input < 0) {
		perror("[~][user input] Can't create the socket");
		return -1;
	}

	if (bind(sock_user_input, (struct sockaddr*)&addr_user_input, sizeof(struct sockaddr_un)) < 0) {
		perror("[~][user input] Can't bind the socket");
		close(sock_user_input);
		unlink(USERINPUT_FILENAME);
		return -1;
	}
*/
	// right now it is just 2 int
	user_input_channel = jakopter_com_add_channel(CHANNEL_USERINPUT, 2*sizeof(int));
	jakopter_com_write_int(user_input_channel, 0, 0);
	jakopter_com_write_int(user_input_channel, 4, 0);

	if (pthread_create(&user_input_thread, NULL, user_input_routine, NULL) < 0) {
		perror("[~][user_input] Can't create thread");
		return -1;
	}

	int i = 0;
/*	while (!recv_ready && i < USERINPUT_TIMEOUT) {
		usleep(500);
		i++;
	}
*/
	return -(i >= USERINPUT_TIMEOUT);
}

int user_input_disconnect()
{
	if (!stopped_user_input) {
		stopped_user_input = true;
		int ret = pthread_join(user_input_thread, NULL);

		//close(fdpipe);
		jakopter_com_remove_channel(CHANNEL_USERINPUT);

		return ret;
	}
	else {
		fprintf(stderr, "[~][user_input] Communication already stopped\n");
		return -1;
	}
}

