#include "common.h"
#include "drone.h"
#include "navdata.h"
#include "user_input.h"
#include "coords.h"

/* The string sent to the drone.*/
static char command[PACKET_SIZE];
/* REF arguments.*/
static char *takeoff_arg = "290718208",
	 *land_arg = "290717696",
	 *emergency_arg = "290717952";

/* Current sequence number.*/
static int cmd_no_sq = 0;
/* Command currently sent.*/
static char *command_type = NULL;
static char command_args[ARGS_MAX][SIZE_ARG];

/* Waiting time spend by command function */
static struct timespec cmd_wait = {0, NAVDATA_ATTEMPT*TIMEOUT_CMD};

/* Thread which send regularly commands to keep the connection.*/
pthread_t cmd_thread;
/* Guard that stops any function if connection isn't initialized.*/
volatile int stopped = 1;
/* Race condition between setting a command and send routine.*/
static pthread_mutex_t mutex_cmd = PTHREAD_MUTEX_INITIALIZER;
/* Race condition between send routine and disconnection.*/
static pthread_mutex_t mutex_stopped = PTHREAD_MUTEX_INITIALIZER;

/**
 * \brief Change the current command sent.
 * \param cmd_type header as AT*SOMETHING
 * \param args arguments of the command
 * \param nb_args number of arguments
 * \returns -1 if nb_args greater than the max number of arguments
*/
int set_cmd(char* cmd_type, char** args, int nb_args)
{
	if (nb_args > ARGS_MAX)
		return -1;

	pthread_mutex_lock(&mutex_cmd);
	command_type = cmd_type;

	int i = 0;
	for (i = 0; i < nb_args; i++) {
		strncpy(command_args[i], args[i], SIZE_ARG);
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
int send_cmd()
{
	int ret;
	pthread_mutex_lock(&mutex_cmd);

	if (command_type != NULL) {
		memset(command, 0, PACKET_SIZE);
		command[0] = '\0';

		char buf[SIZE_INT];
		snprintf(buf, SIZE_INT, "%d", cmd_no_sq);

		strncat(command, "AT*", 4);
		strncat(command, command_type, SIZE_TYPE);
		strncat(command, "=", 2);
		strncat(command, buf, SIZE_INT);

		int i = 0;

		while ((i < ARGS_MAX) && (command_args[i][0] != '\0')) {
			strncat(command, ",", 2);
			strncat(command, command_args[i], SIZE_ARG);
			i++;
		}

		strncat(command, "\r", 2);

		cmd_no_sq++;

		ret = sendto(sock_cmd, command, PACKET_SIZE, 0, (struct sockaddr*)&addr_drone, sizeof(addr_drone));

		pthread_mutex_unlock(&mutex_cmd);

		return ret;
	}
	pthread_mutex_unlock(&mutex_cmd);
	return 0;
}

/**
 * \brief Command used by navdata to define the type of navdata to "demo".
 * \returns send return code or -1 if command couldn't be set.
*/
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

/**
 * \brief Command used by navdata to acknowledge navdata settings.
 * \returns send return code or -1 if command couldn't be set.
*/
int init_navdata_ack()
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
			perror("[~] Can't send command to the drone. \n");
		nanosleep(&itv, NULL);
		itv.tv_nsec = TIMEOUT_CMD;

		pthread_mutex_lock(&mutex_stopped);
	}

	pthread_mutex_unlock(&mutex_stopped);

	pthread_exit(NULL);
}

/**
 * \brief Creates a socket and starts the command thread. Needs the computer to be connected to the drone wifi network.
 * \returns 0 if success, -1 if error
*/
int jakopter_connect()
{
	pthread_mutex_lock(&mutex_stopped);
	if (!stopped) {
		pthread_mutex_unlock(&mutex_stopped);
		perror("[~] Connection already done \n");
		return -1;
	}
	pthread_mutex_unlock(&mutex_stopped);

	addr_drone.sin_family      = AF_INET;
	addr_drone.sin_addr.s_addr = inet_addr(WIFI_ARDRONE_IP);
	addr_drone.sin_port        = htons(PORT_CMD);

	addr_client.sin_family      = AF_INET;
	addr_client.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_client.sin_port        = htons(PORT_CMD);

	sock_cmd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock_cmd < 0) {
		fprintf(stderr, "[~] Can't establish socket \n");
		return -1;
	}

	if (bind(sock_cmd, (struct sockaddr*)&addr_client, sizeof(addr_client)) < 0) {
		fprintf(stderr, "[~] Can't bind socket to port %d\n", PORT_CMD);
		return -1;
	}

	//reinitialize commands
	pthread_mutex_lock(&mutex_cmd);
	cmd_no_sq = 1;
	command_type = NULL;
	pthread_mutex_unlock(&mutex_cmd);

	pthread_mutex_lock(&mutex_stopped);
	stopped = 0;
	pthread_mutex_unlock(&mutex_stopped);

	int navdata_status = navdata_connect();
	if (navdata_status == -1) {
		perror("[~] Navdata connection failed");
		return -1;
	}

	int input_status = user_input_connect();

	if (input_status < 0) {
		perror("[~] Input connection failed");
		return -1;
	}

	int coords_status = coords_connect();

	if (coords_status < 0) {
		perror("[~] Coords connection failed");
		return -1;
	}


	//start the thread
	if (pthread_create(&cmd_thread, NULL, cmd_routine, NULL) < 0) {
		perror("[~] Can't create thread");
		return -1;
	}

	return 0;
}

/**
  * \brief Set the frame of reference of the drone before taking off
  * \returns 0 if success, -1 if the drone is flying or the command couldn't be set.
  */
int jakopter_flat_trim()
{
	if (jakopter_is_flying()) {
		fprintf(stderr, "[*] Drone is flying, setting of frame of reference canceled.\n");
		return -1;
	}
	if (set_cmd(HEAD_FTRIM, NULL, 0) < 0)
		return -1;

	nanosleep(&cmd_wait, NULL);

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return 0;
}

/**
  * \brief Calibration of the drone for smartphone accelerometer
  * \returns 0 if success, -1 if the drone isn't flying or the command couldn't be set.
  */
int jakopter_calib()
{
	if (!jakopter_is_flying()) {
		fprintf(stderr, "[*] Drone isn't flying, calibration canceled.\n");
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

/**
  * \brief Command to take off the drone
  * \returns 0 if success, -1 if error.
  */
int jakopter_takeoff()
{
	if (jakopter_flat_trim() < 0) {
		fprintf(stderr, "[~] Can't establish frame of reference.\n");
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

/**
  * \brief Command to land the drone. If no recent navdata are received, it sends the emergency command.
  * \returns 0 if success, -1 if error.
  */
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

/**
  * \brief Command to stop drone rotors.
  * \returns 0 if success, -1 if command couldn't be set.
  */
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

/**
  * \brief Command to make the drone stay at its position.
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_stay()
{
	char * args[5] = {"0","0","0","0","0"};
	if (set_cmd(HEAD_PCMD, args, 5) < 0)
		return -1;

	nanosleep(&cmd_wait, NULL);

	return 0;
}

/**
  * \brief Command to make the drone rotate to the left with an angular speed.
  * \param speed the angular speed in percentage between 0 and 1
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_rotate_left(float speed)
{
	int ret = jakopter_move(0, 0, 0, -speed);

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return ret;
}

/**
  * \brief Command to make the drone rotate to the right with an angular speed.
  * \param speed the angular speed in percentage between 0 and 1
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_rotate_right(float speed)
{
	int ret = jakopter_move(0, 0, 0, speed);

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return ret;
}

/**
  * \brief Command to make the drone go forward with a defined speed.
  * \param speed the speed in percentage between 0 and 1
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_forward(float speed)
{
	int ret = jakopter_move(0, -speed, 0, 0);

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return ret;
}

/**
  * \brief Command to make the drone go backward with a defined speed.
  * \param speed the speed in percentage between 0 and 1
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_backward(float speed)
{
	int ret = jakopter_move(0, speed, 0, 0);

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return ret;
}

/**
  * \brief Command to make the drone go up with a defined speed.
  * \param speed the speed in percentage between 0 and 1
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_up(float speed)
{
	int ret = jakopter_move(0, 0, speed, 0);

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return ret;
}

/**
  * \brief Command to make the drone go down with a defined speed.
  * \param speed the speed in percentage between 0 and 1
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_down(float speed)
{
	int ret = jakopter_move(0, 0, -speed, 0);

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return ret;
}

/**
  * \brief Command to reset the communication watchdog.
  * \returns 0 if success, -1 if command couldn't be set.
  */
int jakopter_reinit()
{
	if (set_cmd(HEAD_COM_WATCHDOG, NULL, 0) < 0)
		return -1;

	nanosleep(&cmd_wait, NULL);

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return 0;
}
/**
  * \brief define the movement of the drone
  * \param l_to_r the speed from left to right
  * \param f_to_b the speed from forward to backward
  * \param vertical_speed the speed from down to up
  * \param angular_speed the angular speed to rotate the drone
  * \return 0 if success, -1 if command couldn't be set.
  */
int jakopter_move(float l_to_r, float f_to_b, float vertical_speed, float angular_speed)
{
	char * args[5];
	args[0] = "1";

	char buf[SIZE_INT];
	snprintf(buf, SIZE_INT, "%d", *((int *) &l_to_r));
	args[1] = buf;

	char buf2[SIZE_INT];
	snprintf(buf2, SIZE_INT, "%d", *((int *) &f_to_b));
	args[2] = buf2;

	char buf3[SIZE_INT];
	snprintf(buf3, SIZE_INT, "%d", *((int *) &vertical_speed));
	args[3] = buf3;

	char buf4[SIZE_INT];
	snprintf(buf4, SIZE_INT, "%d", *((int *) &angular_speed));
	args[4] = buf4;

	if (set_cmd(HEAD_PCMD, args, 5) < 0)
		return -1;

	nanosleep(&cmd_wait, NULL);

	return 0;
}

/**
  * \brief Stop main thread (End of drone connection)
  * \return pthread_join value or -1 if the communication is already stopped
  */
int jakopter_disconnect()
{
	pthread_mutex_lock(&mutex_stopped);
	if (navdata_disconnect() == 0 && user_input_disconnect() == 0
		&& coords_disconnect() && !stopped) {
		stopped = 1;
		pthread_mutex_unlock(&mutex_stopped);

		close(sock_cmd);
		return pthread_join(cmd_thread, NULL);
	}
	else {
		pthread_mutex_unlock(&mutex_stopped);
		fprintf(stderr, "[~] Communication is already stopped\n");
		return -1;
	}
}