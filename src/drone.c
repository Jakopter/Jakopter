#include <time.h>

#include "common.h"
#include "drone.h"
#include "navdata.h"
#include "user_input.h"

#define LOG_LEN TSTAMP_LEN+PACKET_SIZE

/* The string sent to the drone.*/
static char command[PACKET_SIZE];
/* The last command sent to the drone with a timestamp.*/
static char command_logged[LOG_LEN];
/* REF arguments.*/
static char *takeoff_arg = "290718208",
	*land_arg = "290717696",
	*emergency_arg = "290717952";

/* Current sequence number.*/
static int cmd_no_sq = 0;
/* Command currently sent.*/
static char *command_type = NULL;
static char command_args[ARGS_MAX][ARG_LEN];

/* Waiting time spend by command function */
static struct timespec cmd_wait = {0, NAVDATA_ATTEMPT*TIMEOUT_CMD};

/* Thread which send regularly commands to keep the connection.*/
pthread_t cmd_thread;
/* Guard that stops any function if connection isn't initialized.*/
volatile int stopped = 1;
/* Race condition between setting a command, access for log and send routine.*/
static pthread_mutex_t mutex_cmd = PTHREAD_MUTEX_INITIALIZER;
/* Race condition between send routine and disconnection.*/
static pthread_mutex_t mutex_stopped = PTHREAD_MUTEX_INITIALIZER;

/**
 * \brief Change the current command sent.
 * \param cmd_type header like SOMETHING corresponding to AT*SOMETHING
 * \param args arguments of the command
 * \param nb_args number of arguments
 * \returns -1 if nb_args greater than the max number of arguments
*/
static int set_cmd(char* cmd_type, char** args, int nb_args)
{
	if (nb_args > ARGS_MAX)
		return -1;

	pthread_mutex_lock(&mutex_cmd);
	command_type = cmd_type;

	int i = 0;
	for (i = 0; i < nb_args; i++) {
		strncpy(command_args[i], args[i], ARG_LEN);
	}

	if (i < ARGS_MAX)
		command_args[i][0] = '\0';

	pthread_mutex_unlock(&mutex_cmd);
	return 0;
}


/**
 * \brief Send the current command stored in command_type.
 * \returns sendto return code or 0 if nothing sent
*/
static int send_cmd()
{
	int ret;

	pthread_mutex_lock(&mutex_cmd);

	if (command_type != NULL) {
		memset(command, 0, PACKET_SIZE);
		command[0] = '\0';

		/* Header */
		char buf[INT_LEN];
		snprintf(buf, INT_LEN, "%d", cmd_no_sq);

		strncat(command, "AT*", 4);
		strncat(command, command_type, HEAD_LEN);
		strncat(command, "=", 2);
		strncat(command, buf, INT_LEN);

		/* Options */
		int i = 0;
		while ((i < ARGS_MAX) && (command_args[i][0] != '\0')) {
			strncat(command, ",", 2);
			strncat(command, command_args[i], ARG_LEN);
			i++;
		}

		strncat(command, "\r", 2);

		cmd_no_sq++;

		ret = sendto(sock_cmd, command, PACKET_SIZE, 0, (struct sockaddr*)&addr_drone, sizeof(addr_drone));

		/* Log */
		memset(command_logged, 0, LOG_LEN);
		//unsigned long ts = (unsigned)time(NULL);
		struct timespec ts = {0,0};
		char buf_log[TSTAMP_LEN];
		clock_gettime(CLOCK_REALTIME, &ts);
		snprintf(buf_log, TSTAMP_LEN, "%lu.%lu:", ts.tv_sec, ts.tv_nsec);
		strncat(command_logged, buf_log, TSTAMP_LEN);
		strncat(command_logged, command, PACKET_SIZE);

		pthread_mutex_unlock(&mutex_cmd);

		return ret;
	}
	pthread_mutex_unlock(&mutex_cmd);
	return 0;
}

int init_navdata_bootstrap()
{
	int ret;
	char * bootstrap_cmd[] = {"\"general:navdata_demo\"","\"TRUE\""};

	if (set_cmd(HEAD_CONFIG, bootstrap_cmd, 2) < 0)
		return -1;

	ret = send_cmd();
	nanosleep(&cmd_wait, NULL);

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return ret;
}

int config_ack()
{
	int ret;
	//5 to reset navdata mask
	//Send ACK_CONTROL_MODE
	char * ctrl_cmd[] = {"5","0"};

	if (set_cmd(HEAD_CTRL, ctrl_cmd, 2) < 0)
		return -1;

	ret = send_cmd();
	nanosleep(&cmd_wait, NULL);

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return ret;
}

/**
 * \brief This cmd_thread function is a timer which send a command each TIMEOUT_CMD ns.
 * \param args not used
*/
void* cmd_routine(void* args)
{
	struct timespec itv = {0, TIMEOUT_CMD};
	pthread_mutex_lock(&mutex_stopped);

	while (!stopped) {
		pthread_mutex_unlock(&mutex_stopped);

		if (send_cmd() < 0)
			perror("[~][cmd] Can't send command to the drone");
		nanosleep(&itv, NULL);
		itv.tv_nsec = TIMEOUT_CMD;

		pthread_mutex_lock(&mutex_stopped);
	}

	pthread_mutex_unlock(&mutex_stopped);

	pthread_exit(NULL);
}

int jakopter_connect(const char* drone_ip)
{
	pthread_mutex_lock(&mutex_stopped);
	if (!stopped) {
		pthread_mutex_unlock(&mutex_stopped);
		perror("[~][cmd] Connection already done");
		return -1;
	}
	pthread_mutex_unlock(&mutex_stopped);

	/* Adresses settings */
	addr_drone.sin_family      = AF_INET;
	if (drone_ip == NULL)
		addr_drone.sin_addr.s_addr = inet_addr(WIFI_ARDRONE_IP);
	else
		addr_drone.sin_addr.s_addr = inet_addr(drone_ip);

	if (addr_drone.sin_addr.s_addr == INADDR_NONE) {
		perror("[~][cmd] The drone adress is invalid");
		return -1;
	}
	addr_drone.sin_port        = htons(PORT_CMD);

	addr_client.sin_family      = AF_INET;
	addr_client.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_client.sin_port        = htons(PORT_CMD);

	sock_cmd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock_cmd < 0) {
		fprintf(stderr, "[~][cmd] Can't establish socket \n");
		return -1;
	}

	if (bind(sock_cmd, (struct sockaddr*)&addr_client, sizeof(addr_client)) < 0) {
		fprintf(stderr, "[~][cmd] Can't bind socket to port %d\n", PORT_CMD);
		return -1;
	}

	/* Reinitialize commands */
	pthread_mutex_lock(&mutex_cmd);
	cmd_no_sq = 1;
	command_type = NULL;
	memset(command_logged, 0, INT_LEN+1+PACKET_SIZE);
	pthread_mutex_unlock(&mutex_cmd);

	pthread_mutex_lock(&mutex_stopped);
	stopped = 0;
	pthread_mutex_unlock(&mutex_stopped);

	/* Modules */
	int navdata_status = navdata_connect();
	if (navdata_status == -1) {
		perror("[~][cmd] Navdata connection failed");
		return -1;
	}

	int input_status = user_input_connect();
	if (input_status < 0) {
		perror("[~][cmd] Input connection failed");
		return -1;
	}

	/* Main thread */
	if (pthread_create(&cmd_thread, NULL, cmd_routine, NULL) < 0) {
		perror("[~][cmd] Can't create thread");
		return -1;
	}

	return 0;
}

int jakopter_disconnect()
{
	pthread_mutex_lock(&mutex_stopped);
	if (navdata_disconnect() == 0 && user_input_disconnect() == 0 && !stopped) {
		stopped = 1;
		pthread_mutex_unlock(&mutex_stopped);

		close(sock_cmd);
		return pthread_join(cmd_thread, NULL);
	}
	else {
		pthread_mutex_unlock(&mutex_stopped);
		fprintf(stderr, "[~][cmd] Communication is already stopped\n");
		return -1;
	}
}

int jakopter_flat_trim()
{
	if (jakopter_is_flying()) {
		fprintf(stderr, "[*][cmd] Drone is flying, setting of frame of reference canceled.\n");
		return -1;
	}
	if (set_cmd(HEAD_FTRIM, NULL, 0) < 0)
		return -1;

	nanosleep(&cmd_wait, NULL);

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return 0;
}

int jakopter_calib()
{
	if (!jakopter_is_flying()) {
		fprintf(stderr, "[*][cmd] Drone isn't flying, calibration canceled.\n");
		return -1;
	}

	char * args[] = {"0"};

	if (set_cmd(HEAD_CALIB, args, 1) < 0)
		return -1;

	nanosleep(&cmd_wait, NULL);

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return 0;
}

int jakopter_switch_camera(unsigned int id)
{
	if (id >= VID_CHANNELS) {
		fprintf(stderr, "[*][cmd] Invalid id for the camera.\n");
		return -1;
	}

	static char buf[INT_LEN+2];
	snprintf(buf, INT_LEN, "\"%u\"", id);

	char * args[] = {"\"video:video_channel\"", buf};

	if (set_cmd(HEAD_CONFIG, args, 2) < 0)
		return -1;

	nanosleep(&cmd_wait, NULL);

	// if (config_ack() < 0)
	// 	return -1;
	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return 0;
}

int jakopter_reinit()
{
	if (set_cmd(HEAD_COM_WATCHDOG, NULL, 0) < 0)
		return -1;

	nanosleep(&cmd_wait, NULL);

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return 0;
}

int jakopter_takeoff()
{
	if (jakopter_flat_trim() < 0) {
		fprintf(stderr, "[~][cmd] Can't establish frame of reference.\n");
		return -1;
	}

	char * args[] = {takeoff_arg};
	set_cmd(HEAD_REF, args, 1);

	//set timeout
	int no_sq = 0;
	no_sq = navdata_no_sq();
	int attempt = 0;

	//wait navdata start
	while(no_sq == 0 && attempt < NAVDATA_ATTEMPT) {
		nanosleep(&cmd_wait, NULL);
		no_sq = navdata_no_sq();
		attempt++;
	}

	attempt = 0;

	while(attempt < NAVDATA_ATTEMPT &&
		(!jakopter_is_flying() || jakopter_height() < HEIGHT_THRESHOLD))
	{
		nanosleep(&cmd_wait, NULL);
		attempt++;
	}

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return 0;
}

int jakopter_land()
{
	char * args[] = {land_arg};
	set_cmd(HEAD_REF, args, 1);

	int no_sq = 0;
	int attempt = 0;

	while (no_sq == 0 && attempt < NAVDATA_ATTEMPT) {
		nanosleep(&cmd_wait, NULL);
		no_sq = navdata_no_sq();
		attempt++;
	}

	int init_no_sq = navdata_no_sq();
	int emergency = 0;
	attempt = 0;

	while (jakopter_is_flying() && jakopter_height() > HEIGHT_THRESHOLD && !emergency) {
		nanosleep(&cmd_wait, NULL);
		no_sq = navdata_no_sq();

		//emergency sent if no new data received
		if (no_sq == init_no_sq && attempt >= NAVDATA_ATTEMPT)
			emergency = jakopter_emergency();

		attempt++;
	}

	set_cmd(NULL, NULL, 0);

	return 0;
}

int jakopter_emergency()
{
	char * args[] = {emergency_arg};
	if (set_cmd(HEAD_REF, args, 1) < 0)
		return -1;

	nanosleep(&cmd_wait, NULL);

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return 0;
}

int jakopter_stay()
{
	char * args[5] = {"0","0","0","0","0"};
	if (set_cmd(HEAD_PCMD, args, 5) < 0)
		return -1;

	nanosleep(&cmd_wait, NULL);

	return 0;
}

int jakopter_rotate_left(float speed)
{
	int ret = jakopter_move(0, 0, 0, -speed);

	if (jakopter_stay() < 0)
		return -1;

	return ret;
}

int jakopter_rotate_right(float speed)
{
	int ret = jakopter_move(0, 0, 0, speed);

	if (jakopter_stay() < 0)
		return -1;

	return ret;
}

int jakopter_slide_left(float speed)
{
	int ret = jakopter_move(-speed, 0, 0, 0);

	if (jakopter_stay() < 0)
		return -1;

	return ret;
}

int jakopter_slide_right(float speed)
{
	int ret = jakopter_move(speed, 0, 0, 0);

	if (jakopter_stay() < 0)
		return -1;

	return ret;
}

int jakopter_forward(float speed)
{
	int ret = jakopter_move(0, speed, 0, 0);

	if (jakopter_stay() < 0)
		return -1;

	return ret;
}

int jakopter_backward(float speed)
{
	int ret = jakopter_move(0, -speed, 0, 0);

	if (jakopter_stay() < 0)
		return -1;

	return ret;
}

int jakopter_up(float speed)
{
	int ret = jakopter_move(0, 0, speed, 0);

	if (jakopter_stay() < 0)
		return -1;

	return ret;
}

int jakopter_down(float speed)
{
	int ret = jakopter_move(0, 0, -speed, 0);

	if (jakopter_stay() < 0)
		return -1;

	return ret;
}

int jakopter_move(float l_to_r, float b_to_f, float vertical_speed, float angular_speed)
{
	//inverted in Parrot ARdrone Protocol
	if (b_to_f != 0.0) {
		b_to_f = -b_to_f;
	}
	char * args[5];
	args[0] = "1";

	char buf[INT_LEN];
	snprintf(buf, INT_LEN, "%d", *((int *) &l_to_r));
	args[1] = buf;

	char buf2[INT_LEN];
	snprintf(buf2, INT_LEN, "%d", *((int *) &b_to_f));
	args[2] = buf2;

	char buf3[INT_LEN];
	snprintf(buf3, INT_LEN, "%d", *((int *) &vertical_speed));
	args[3] = buf3;

	char buf4[INT_LEN];
	snprintf(buf4, INT_LEN, "%d", *((int *) &angular_speed));
	args[4] = buf4;

	if (set_cmd(HEAD_PCMD, args, 5) < 0)
		return -1;

	nanosleep(&cmd_wait, NULL);

	return 0;
}

const char* jakopter_log_command()
{
	static char ret[LOG_LEN+1];
	if (!stopped) {
		pthread_mutex_lock(&mutex_cmd);
		strncpy(ret, command_logged, LOG_LEN+1);
		pthread_mutex_unlock(&mutex_cmd);
		return ret;
	}
	else
		return NULL;
}