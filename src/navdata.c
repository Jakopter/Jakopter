#include "common.h"
#include "navdata.h"
#include "drone.h"

/* The structure which contains navdata  */
static union navdata_t data;

jakopter_com_channel_t* nav_channel;

pthread_t navdata_thread;

/* Guard that stops any function if connection isn't initialized.*/
static bool stopped_navdata = true;
/* Race condition between navdata reception and read the navdata.*/
static pthread_mutex_t mutex_navdata = PTHREAD_MUTEX_INITIALIZER;
/* Race condition between receive routine and disconnection.*/
static pthread_mutex_t mutex_stopped = PTHREAD_MUTEX_INITIALIZER;

/* Drone address + client address (required to set the port number)*/
static struct sockaddr_in addr_drone_navdata, addr_client_navdata;
static int sock_navdata;

/**
  * \brief Receive the navdata from the drone and write it in NAVDATA_CHANNEL.
  * \return the result of recvfrom
  */
static int recv_cmd()
{
	pthread_mutex_lock(&mutex_navdata);
	socklen_t len = sizeof(addr_drone_navdata);
	int ret = recvfrom(sock_navdata, &data, sizeof(data), 0, (struct sockaddr*)&addr_drone_navdata, &len);
	size_t offset = 0;

	switch (data.demo.tag) {
		case TAG_DEMO:
			jakopter_com_write_int(nav_channel, offset, data.demo.vbat_flying_percentage);
			offset += sizeof(data.demo.vbat_flying_percentage);
			jakopter_com_write_int(nav_channel, offset, data.demo.altitude);
			offset += sizeof(data.demo.altitude);
			jakopter_com_write_float(nav_channel, offset, data.demo.theta);
			offset += sizeof(data.demo.theta);
			jakopter_com_write_float(nav_channel, offset, data.demo.phi);
			offset += sizeof(data.demo.phi);
			jakopter_com_write_float(nav_channel, offset, data.demo.psi);
			offset += sizeof(data.demo.psi);
			jakopter_com_write_float(nav_channel, offset, data.demo.vx);
			offset += sizeof(data.demo.vx);
			jakopter_com_write_float(nav_channel, offset, data.demo.vy);
			offset += sizeof(data.demo.vy);
			jakopter_com_write_float(nav_channel, offset, data.demo.vz);
			break;
		default:
			break;
	}

	pthread_mutex_unlock(&mutex_navdata);

	return ret;
}

/**
  * \brief Procedure to initialize the communication of navdata with the drone.
  * \return 0 if success, -1 if an error occured
  */
static int navdata_init()
{
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(sock_navdata, &fds);
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	if (sendto(sock_navdata, "\x01", 1, 0, (struct sockaddr*)&addr_drone_navdata, sizeof(addr_drone_navdata)) < 0) {
		perror("[~][navdata] Can't send ping\n");
		return -1;
	}

	if (select(sock_navdata+1, &fds, NULL, NULL, &timeout) <= 0) {
		perror("[~][navdata] Ping ack not received\n");
		return -1;
	}


	if (recv_cmd() < 0) {
		perror("[~][navdata] First navdata packet not received\n");
		return -1;
	}

	if (data.raw.ardrone_state & (1 << 11)) {
		fprintf(stderr, "[*][navdata] bootstrap: %d\n", (data.raw.ardrone_state & (1 << 11))%2);
	}

	if (data.raw.ardrone_state & (1 << 15)) {
		fprintf(stderr, "[*][navdata] Battery charge too low: %d\n", data.raw.ardrone_state & (1 << 15));
		return -1;
	}

	if (init_navdata_bootstrap() < 0){
		fprintf(stderr, "[~][navdata] bootstrap init failed\n");
		return -1;
	}

	if (recv_cmd() < 0)
		perror("[~][navdata] Second navdata packet not received");

	if (data.raw.ardrone_state & (1 << 6)) {
		fprintf(stderr, "[*][navdata] control command ACK: %d\n", (data.raw.ardrone_state & (1 << 6))%2);
	}

	if (init_navdata_ack() < 0){
		fprintf(stderr, "[~][navdata] Init ack failed\n");
		return -1;
	}

	if (data.raw.ardrone_state & (1 << 11)) {
		fprintf(stderr, "[~][navdata] bootstrap end: %d\n", (data.raw.ardrone_state & (1 << 11))%2);
	}

	return 0;
}

/**
  * \brief navdata_thread routine which keep the connection alive.
  */
void* navdata_routine(void* args)
{
	pthread_mutex_lock(&mutex_stopped);

	while (!stopped_navdata) {
		pthread_mutex_unlock(&mutex_stopped);

		if (recv_cmd() < 0)
			perror("[~][navdata] Failed to receive navdata");
		usleep(NAVDATA_INTERVAL*1000);

		if (sendto(sock_navdata, "\x01", 1, 0, (struct sockaddr*)&addr_drone_navdata, sizeof(addr_drone_navdata)) < 0) {
			perror("[~][navdata] Failed to send ping\n");
			pthread_exit(NULL);
		}

		pthread_mutex_lock(&mutex_stopped);
	}

	pthread_mutex_unlock(&mutex_stopped);

	pthread_exit(NULL);
}

/**
  * \brief Start navdata thread
  * \return 0 if success, -1 if error
  */
int navdata_connect()
{
	if (!stopped_navdata)
		return -1;

	addr_drone_navdata.sin_family      = AF_INET;
	addr_drone_navdata.sin_addr.s_addr = inet_addr(WIFI_ARDRONE_IP);
	addr_drone_navdata.sin_port        = htons(PORT_NAVDATA);

	addr_client_navdata.sin_family      = AF_INET;
	addr_client_navdata.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_client_navdata.sin_port        = htons(PORT_NAVDATA);

	sock_navdata = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (sock_navdata < 0) {
		fprintf(stderr, "[~][navdata] Can't establish socket \n");
		return -1;
	}

	if (bind(sock_navdata, (struct sockaddr*)&addr_client_navdata, sizeof(addr_client_navdata)) < 0) {
		fprintf(stderr, "[~][navdata] Can't bind socket to port %d\n", PORT_NAVDATA);
		return -1;
	}

	nav_channel = jakopter_com_add_channel(CHANNEL_NAVDATA, sizeof(data));

	if (navdata_init() < 0) {
		perror("[~][navdata] Init sequence failed");
		return -1;
	}

	pthread_mutex_lock(&mutex_stopped);
	stopped_navdata = false;
	pthread_mutex_unlock(&mutex_stopped);

	if(pthread_create(&navdata_thread, NULL, navdata_routine, NULL) < 0) {
		perror("[~][navdata] Can't create thread");
		return -1;
	}

	return 0;
}

/**
  * \return a boolean
  */
int jakopter_is_flying()
{
	int flyState = -1;
	pthread_mutex_lock(&mutex_navdata);
	flyState = data.raw.ardrone_state & 0x0001;
	pthread_mutex_unlock(&mutex_navdata);
	return flyState;
}

/**
  * \return the height in millimeters or -1 if navdata are not received
  */
int jakopter_height()
{
	int height = -1;

	if (data.raw.options[0].tag != TAG_DEMO && data.raw.sequence < 1) {
		perror("[~][navdata] Current tag does not match TAG_DEMO.");
		return height;
	}

	pthread_mutex_lock(&mutex_navdata);
	height = data.demo.altitude;
	pthread_mutex_unlock(&mutex_navdata);

	return height;
}

/**
  * \return the percentage of the relative angle between -1.0 and 1.0 or -2.0 if navdata are not received
  */
float jakopter_y_axis()
{
	float y_axis = -2.0;

	if (data.raw.options[0].tag != TAG_DEMO && data.raw.sequence < 1) {
		perror("[~][navdata] Current tag does not match TAG_DEMO.");
		return y_axis;
	}

	pthread_mutex_lock(&mutex_navdata);
	y_axis = data.demo.psi;
	pthread_mutex_unlock(&mutex_navdata);

	return y_axis;
}

/**
  * \return the sequence number of navdata
  */
int navdata_no_sq()
{
	int ret;
	pthread_mutex_lock(&mutex_navdata);
	ret = data.raw.sequence;
	pthread_mutex_unlock(&mutex_navdata);
	return ret;
}

/**
  * \brief Stop navdata thread.
  * \return the pthread_join value or -1 if communication already stopped.
  */
int navdata_disconnect()
{
	int ret;
	pthread_mutex_lock(&mutex_stopped);

	if (!stopped_navdata) {
		stopped_navdata = true;
		pthread_mutex_unlock(&mutex_stopped);
		ret = pthread_join(navdata_thread, NULL);

		jakopter_com_remove_channel(CHANNEL_NAVDATA);

		close(sock_navdata);
	}
	else {
		pthread_mutex_unlock(&mutex_stopped);

		fprintf(stderr, "[~][navdata] Communication already stopped\n");
		ret = -1;
	}

	return ret;
}

/**
  * \brief Print the content of received navdata
  */
void debug_navdata_demo()
{
	pthread_mutex_lock(&mutex_navdata);
	printf("Header: %x\n",data.demo.header);
	printf("Mask: %x\n",data.demo.ardrone_state);
	printf("Sequence num: %d\n",data.demo.sequence);
	printf("Tag: %x\n",data.demo.tag);
	printf("Size: %d\n",data.demo.size);
	printf("Fly state: %x\n",data.demo.ctrl_state); //Masque defined in ctrl_states.h
	printf("Theta: %f\n",data.demo.theta);
	printf("Phi: %f\n",data.demo.phi);
	printf("Psi: %f\n",data.demo.psi);//Yaw
	pthread_mutex_unlock(&mutex_navdata);
}
