#include "common.h"
#include "navdata.h"
#include "drone.h"

#define LOG_LEN TSTAMP_LEN+DEMO_LEN+1

/* The structure which contains navdata  */
static union navdata_t data;
/* The string which contains the timestamp of the last request.*/
static char timestamp[TSTAMP_LEN];

jakopter_com_channel_t* nav_channel;

pthread_t navdata_thread;

/* Guard that stops any function if connection isn't initialized.*/
static bool stopped_navdata = true;
/* Race condition between navdata reception and read the navdata.*/
static pthread_mutex_t mutex_navdata = PTHREAD_MUTEX_INITIALIZER;
/* Race condition between receive routine and disconnection.*/
static pthread_mutex_t mutex_stopped = PTHREAD_MUTEX_INITIALIZER;
/* Race condition between requesting timestamp and record of timestamp*/
static pthread_mutex_t mutex_timestamp = PTHREAD_MUTEX_INITIALIZER;

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

static void navdata_timestamp() {
	pthread_mutex_lock(&mutex_timestamp);
	memset(timestamp, 0, TSTAMP_LEN+1);
	struct timespec ts = {0,0};
	clock_gettime(CLOCK_REALTIME, &ts);
	snprintf(timestamp, TSTAMP_LEN+1, "%lu.%lu:", ts.tv_sec, ts.tv_nsec);
	pthread_mutex_unlock(&mutex_timestamp);
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
		fprintf(stderr, "[*][navdata] bootstrap: %d\n", (data.raw.ardrone_state >> 11) & 1);
	}

	if (data.raw.ardrone_state & (1 << 15)) {
		fprintf(stderr, "[*][navdata] Battery charge too low: %d\n", (data.raw.ardrone_state >> 15) & 1);
		//TODO: Use errcode instead
		//return -1;
	}

	if (init_navdata_bootstrap() < 0){
		fprintf(stderr, "[~][navdata] bootstrap init failed\n");
		return -1;
	}

	if (recv_cmd() < 0)
		perror("[~][navdata] Second navdata packet not received");

	if (data.raw.ardrone_state & (1 << 6)) {
		fprintf(stderr, "[*][navdata] control command ACK: %d\n", (data.raw.ardrone_state >> 6) & 1);
	}

	if (config_ack() < 0){
		fprintf(stderr, "[~][navdata] Init ack failed\n");
		return -1;
	}

	if (data.raw.ardrone_state & (1 << 11)) {
		fprintf(stderr, "[~][navdata] bootstrap end: %d\n", (data.raw.ardrone_state >> 11) & 1);
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

int navdata_connect(const char* drone_ip)
{
	if (!stopped_navdata)
		return -1;

	addr_drone_navdata.sin_family      = AF_INET;
	if (drone_ip == NULL)
		addr_drone_navdata.sin_addr.s_addr = inet_addr(WIFI_ARDRONE_IP);
	else
		addr_drone_navdata.sin_addr.s_addr = inet_addr(drone_ip);
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

int jakopter_is_flying()
{
	int flyState = -1;
	pthread_mutex_lock(&mutex_navdata);
	flyState = data.raw.ardrone_state & 0x0001;
	pthread_mutex_unlock(&mutex_navdata);
	navdata_timestamp();
	return flyState;
}

int jakopter_battery()
{
	int battery = -1;

	if (data.raw.options[0].tag != TAG_DEMO && data.raw.sequence < 1) {
		fprintf(stderr, "[~][navdata] Current tag does not match TAG_DEMO.");
		return battery;
	}

	pthread_mutex_lock(&mutex_navdata);
	battery = data.demo.vbat_flying_percentage;
	pthread_mutex_unlock(&mutex_navdata);

	navdata_timestamp();

	return battery;
}

int jakopter_height()
{
	int height = -1;

	if (data.raw.options[0].tag != TAG_DEMO && data.raw.sequence < 1) {
		fprintf(stderr, "[~][navdata] Current tag does not match TAG_DEMO.");
		return height;
	}

	pthread_mutex_lock(&mutex_navdata);
	height = data.demo.altitude;
	pthread_mutex_unlock(&mutex_navdata);

	navdata_timestamp();

	return height;
}

int navdata_no_sq()
{
	int ret;
	pthread_mutex_lock(&mutex_navdata);
	ret = data.raw.sequence;
	pthread_mutex_unlock(&mutex_navdata);
	navdata_timestamp();
	return ret;
}

void debug_navdata_demo()
{
	pthread_mutex_lock(&mutex_navdata);
	printf("Header: %x\n",data.demo.header);
	printf("Mask: %x\n",data.demo.ardrone_state);
	printf("Sequence num: %d\n",data.demo.sequence);
	printf("Tag: %x\n",data.demo.tag);
	printf("Size: %d\n",data.demo.size);
	printf("Fly state: %x\n",data.demo.ctrl_state); //Mask defined in SDK ctrl_states.h
	printf("Theta: %f\n",data.demo.theta); //Pitch
	printf("Phi: %f\n",data.demo.phi); //Roll
	printf("Psi: %f\n",data.demo.psi); //Yaw
	pthread_mutex_unlock(&mutex_navdata);
}

const char* jakopter_log_navdata()
{
	static char ret[LOG_LEN];
	if (!stopped_navdata) {
		memset(ret, 0, LOG_LEN);
		char buf[DEMO_LEN];
		pthread_mutex_lock(&mutex_timestamp);
		strncat(ret, timestamp, TSTAMP_LEN);
		pthread_mutex_unlock(&mutex_timestamp);
		pthread_mutex_lock(&mutex_navdata);
		snprintf(buf, DEMO_LEN, "%d %d %d %d %f %f %f %f %f %f ",
			data.demo.ardrone_state,
			data.demo.ctrl_state,
			data.demo.vbat_flying_percentage,
			data.demo.altitude,
			data.demo.theta,
			data.demo.phi,
			data.demo.psi,
			data.demo.vx,
			data.demo.vy,
			data.demo.vz
			);
		pthread_mutex_unlock(&mutex_navdata);
		strncat(ret, buf, DEMO_LEN);
		return ret;
	}
	else
		return NULL;
}