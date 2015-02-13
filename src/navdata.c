#include "common.h"
#include "navdata.h"
#include "drone.h"

static union navdata_t data;

jakopter_com_channel_t* nav_channel;

pthread_t navdata_thread;
bool stopped_navdata = true;      //Guard that stops any function if connection isn't initialized.
static pthread_mutex_t mutex_navdata = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_stopped = PTHREAD_MUTEX_INITIALIZER;


/*Network data*/
/* drone address + client address (required to set the port number)*/
struct sockaddr_in addr_drone_navdata, addr_client_navdata;
int sock_navdata;

int recv_cmd()
{

	pthread_mutex_lock(&mutex_navdata);
	socklen_t len = sizeof(addr_drone_navdata);
	int ret = recvfrom(sock_navdata, &data, sizeof(data), 0, (struct sockaddr*)&addr_drone_navdata, &len);
	//Write
	size_t offset = 0;
	switch(data.demo.tag) {
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
			offset += sizeof(data.demo.vz);
			break;
		default:
			break;
	}
	pthread_mutex_unlock(&mutex_navdata);
	return ret;
}

int navdata_init()
{
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(sock_navdata, &fds);
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	if(sendto(sock_navdata, "\x01", 1, 0, (struct sockaddr*)&addr_drone_navdata, sizeof(addr_drone_navdata)) < 0) {
		perror("[~][navdata] Can't send ping\n");
		return -1;
	}

	if(select(sock_navdata+1, &fds, NULL, NULL, &timeout) <= 0) {
		perror("[~][navdata] Ping ack not received\n");
		return -1;
	}


	if(recv_cmd() < 0) {
		perror("[~][navdata] First navdata packet not received\n");
		return -1;
	}

	if(data.raw.ardrone_state & (1 << 11)) {
		fprintf(stderr, "[*][navdata] bootstrap: %d\n", (data.raw.ardrone_state & (1 << 11))%2);
	}

	if(data.raw.ardrone_state & (1 << 15)) {
		fprintf(stderr, "[*][navdata] Battery charge too low: %d\n", data.raw.ardrone_state & (1 << 15));
		return -1;
	}

	if(init_navdata_bootstrap() < 0){
		fprintf(stderr, "[~][navdata] bootstrap init failed\n");
		return -1;
	}

	if(recv_cmd() < 0)
		perror("[~][navdata] Second navdata packet not received");

	if(data.raw.ardrone_state & (1 << 6)) {
		fprintf(stderr, "[*][navdata] control command ACK: %d\n", (data.raw.ardrone_state & (1 << 6))%2);
	}

	if(init_navdata_ack() < 0){
		fprintf(stderr, "[~][navdata] Init ack failed\n");
		return -1;
	}

	if(data.raw.ardrone_state & (1 << 11)) {
		fprintf(stderr, "[~][navdata] bootstrap end: %d\n", (data.raw.ardrone_state & (1 << 11))%2);
	}

	return 0;
}

/*navdata_thread function*/
void* navdata_routine(void* args)
{
	pthread_mutex_lock(&mutex_stopped);
	while(!stopped_navdata) {
		pthread_mutex_unlock(&mutex_stopped);

		if(recv_cmd() < 0)
			perror("[~][navdata] Failed to receive navdata");
		usleep(NAVDATA_INTERVAL*1000);

		if(sendto(sock_navdata, "\x01", 1, 0, (struct sockaddr*)&addr_drone_navdata, sizeof(addr_drone_navdata)) < 0) {
			perror("[~][navdata] Failed to send ping\n");
			pthread_exit(NULL);
		}

		pthread_mutex_lock(&mutex_stopped);
	}
	pthread_mutex_unlock(&mutex_stopped);

	pthread_exit(NULL);
}

int navdata_connect()
{

	if(!stopped_navdata)
		return -1;

	addr_drone_navdata.sin_family      = AF_INET;
	addr_drone_navdata.sin_addr.s_addr = inet_addr(WIFI_ARDRONE_IP);
	addr_drone_navdata.sin_port        = htons(PORT_NAVDATA);

	addr_client_navdata.sin_family      = AF_INET;
	addr_client_navdata.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_client_navdata.sin_port        = htons(PORT_NAVDATA);

	sock_navdata = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock_navdata < 0) {
		fprintf(stderr, "[~][navdata] Can't establish socket \n");
		return -1;
	}

	if(bind(sock_navdata, (struct sockaddr*)&addr_client_navdata, sizeof(addr_client_navdata)) < 0) {
		fprintf(stderr, "[~][navdata] Can't bind socket to port %d\n", PORT_NAVDATA);
		return -1;
	}

	//com_master doesn't need to be initialized now.
/*	if (!jakopter_com_master_is_init()) {
		perror("[~][navdata] Com channel master not init");
		return -1;
	}*/
	nav_channel = jakopter_com_add_channel(CHANNEL_NAVDATA, sizeof(data));

	if(navdata_init() < 0) {
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


int jakopter_is_flying()
{
	int flyState = -1;
	pthread_mutex_lock(&mutex_navdata);
	flyState = data.raw.ardrone_state & 0x0001;
	pthread_mutex_unlock(&mutex_navdata);
	return flyState;
}



int jakopter_height()
{
	int height = -1;
	if(data.raw.options[0].tag != TAG_DEMO){
		perror("[~][navdata] Current tag does not match TAG_DEMO.");
		return height;
	}
	pthread_mutex_lock(&mutex_navdata);
	height = data.demo.altitude;
	pthread_mutex_unlock(&mutex_navdata);
	return height;
}

float jakopter_y_axis()
{
	float y_axis = -1.0;
	if(data.raw.options[0].tag != TAG_DEMO){
		perror("[~][navdata] Current tag does not match TAG_DEMO.");
		return y_axis;
	}

	pthread_mutex_lock(&mutex_navdata);
	y_axis = data.demo.psi;
	pthread_mutex_unlock(&mutex_navdata);
	return y_axis;
}

int navdata_no_sq()
{
	int ret;
	pthread_mutex_lock(&mutex_navdata);
	ret = data.raw.sequence;
	pthread_mutex_unlock(&mutex_navdata);
	return ret;
}


int navdata_disconnect()
{
	pthread_mutex_lock(&mutex_stopped);
	if(!stopped_navdata) {
		stopped_navdata = true;
		pthread_mutex_unlock(&mutex_stopped);
		int ret = pthread_join(navdata_thread, NULL);

		jakopter_com_remove_channel(1);

		close(sock_navdata);

		return ret;
	}
	else {
		pthread_mutex_unlock(&mutex_stopped);

		fprintf(stderr, "[~][navdata] Communication already stopped\n");
		return -1;
	}

}

void debug_navdata_demo() {
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
