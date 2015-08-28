#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <pthread.h>
#include <iostream>
#include <sys/un.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
 
/* Expected size of the datas received from the virtual drone */
#define BUF_SIZE sizeof(float)*10
/* Arbitrary port on which the virtual drone sends its coords*/
#define PORT 5557
/* File in which write the coords for Jakopter to later read them */
#define COORDS_FILENAME "/tmp/jakopter_coords.txt"

/* Address of the virtual drone (i.e. the computer running the drone simulation) */
static sockaddr_in addr_drone_coords;
/* Address of the computer on which COORDS_FILENAME is located (i.e. localhost) */
static sockaddr_un addr_local_coords;

/* Sockets  to, respectively, write in COORDS_FILENAME and communicate with the virtual drone*/
static int sock_local;
static int sock_drone;

/* Destined for receiving the coords sent by the virutal drone  : position in m x, y, z ; euler angles in radians  rx, ry, rz ; quaternion angles qx,qy,qz,qw*/
static float data[10];

static pthread_t write_in_file_thread;
static pthread_t receive_from_drone_thread;


/* Race condition between navdata reception and read the navdata.*/
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* receive_from_drone(void* test)
{
	pthread_mutex_lock(&mutex);
	socklen_t len = sizeof(addr_drone_coords);
	int ret = recvfrom(sock_drone, &data, sizeof(data), 0, (struct sockaddr*)&addr_drone_coords, &len);
	if(ret != BUF_SIZE)
	{
		perror("[~][coords] Couldn't receive coords from virtual drone");
	}
	usleep(1000);
	pthread_mutex_unlock(&mutex);
	return 0;
}

void* write_in_file(void* test)
{
	//socklen_t len = sizeof(addr_drone_coords);
	pthread_mutex_lock(&mutex);
	int ret = sendto(sock_local, data, sizeof(data), 0, (struct sockaddr*)&addr_local_coords, sizeof(addr_local_coords));
	if(ret != BUF_SIZE)
	{
		perror("[~][coords] Couldn't write coords in jakopter_coords.txt");
	}
	pthread_mutex_unlock(&mutex);
	return 0;
}

int main(int argc, char* argv[])
{
	if((sock_local=socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
	{
		perror("[~][coords] Couldn't initialize socket in AF_UNIX mode");
		return -1;
	}
	if((sock_drone=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		perror("[~][coords]Couldn't initialize socket in AF_INET mode");
		return -1;
	}

	addr_drone_coords.sin_family = AF_INET;
	addr_drone_coords.sin_port = htons(PORT);
	addr_drone_coords.sin_addr.s_addr = htonl(INADDR_ANY);

	memset(&addr_local_coords, '\0', sizeof(struct sockaddr_un));
	addr_local_coords.sun_family = AF_UNIX;
	strncpy(addr_local_coords.sun_path, COORDS_FILENAME, sizeof(addr_local_coords.sun_path)-1);


	if(bind(sock_local, (struct sockaddr*)&addr_local_coords, sizeof(addr_local_coords)) == -1)
	{
		perror("[~][coords] Couldn't bind socket to COORDS_FILENAME");
		return -1;
	}
	if(bind(sock_drone, (struct sockaddr*)&addr_drone_coords, sizeof(addr_drone_coords)) == -1)
	{
		perror("[~][coords] Couldn't bind socket to virtual drone");
		return -1;
	}

	if (pthread_create(&write_in_file_thread, NULL, write_in_file, NULL) < 0) {
		close(sock_local);
		perror("[~][coords] Couldn't create thread");
		return -1;
	}
	if (pthread_create(&receive_from_drone_thread, NULL, receive_from_drone, NULL) < 0) {
		close(sock_drone);
		perror("[~][coords] Couldn't create thread");
		return -1;
	}

	std::cin.get();

	close(sock_drone);
	close(sock_local);


	return 0;

}
