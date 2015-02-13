#include "common.h"
#include "drone.h"
#include "navdata.h"

/*takeoff and land comm*/
char ref_cmd[PACKET_SIZE];
/* REF arguments */
char *takeoff_arg = "290718208",
	 *land_arg = "290717696",
	 *emergency_arg = "290717952";
/* PCMD arguments */

/*Current sequence number*/
int cmd_no_sq = 0;
/*command currently sent*/
char *cmd_current = NULL;
char cmd_current_args[ARGS_MAX][SIZE_ARG];

/* Waiting time spend by command function */
struct timespec cmd_wait = {0, NAVDATA_ATTEMPT*TIMEOUT_CMD};

/*Thread which send regularly commands to keep the connection*/
pthread_t cmd_thread;
/*Guard that stops any function if connection isn't initialized.*/
int stopped = 1;
/* Race condition between setting a command and send routine*/
static pthread_mutex_t mutex_cmd = PTHREAD_MUTEX_INITIALIZER;
/* Race condition between send routine and disconnection */
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
	cmd_current = cmd_type;

	int i = 0;
	for (i = 0; (i < ARGS_MAX) && (i < nb_args); i++) {
		strncpy(cmd_current_args[i], args[i], SIZE_ARG);
	}

	if (i < ARGS_MAX)
		cmd_current_args[i][0] = '\0';

	pthread_mutex_unlock(&mutex_cmd);
	return 0;
}

/**
 * \brief Concatenate command items before dispatch.
 * \param cmd_type header as AT*SOMETHING
 * \param args arguments of the command
 * \param nb_args number of arguments
 * \pre Protected by mutex_cmd
*/
void gen_cmd(char * cmd, char* cmd_type, int no_sq)
{
	char buf[SIZE_INT];
	snprintf(buf, SIZE_INT, "%d", no_sq);

	cmd = strncat(cmd, "AT*", PACKET_SIZE);
	cmd = strncat(cmd, cmd_type, PACKET_SIZE);
	cmd = strncat(cmd, "=", PACKET_SIZE);
	cmd = strncat(cmd, buf, PACKET_SIZE);

	int i = 0;
	while((cmd_current_args[i][0] != '\0') && (i < ARGS_MAX)) {
		cmd = strncat(cmd, ",", PACKET_SIZE);
		cmd = strncat(cmd, cmd_current_args[i], PACKET_SIZE);
		i++;
	}
	cmd = strncat(cmd, "\r", PACKET_SIZE);
}

/**
 * \brief Send the current command stored in cmd_current.
 * \returns sendto return code or 0 if nothing sent
*/
int send_cmd()
{
	int ret;
	pthread_mutex_lock(&mutex_cmd);

	if (cmd_current != NULL) {
		memset(ref_cmd, 0, PACKET_SIZE);
		ref_cmd[0] = '\0';
		gen_cmd(ref_cmd,cmd_current,cmd_no_sq);
		cmd_no_sq++;
		
		ret = sendto(sock_cmd, ref_cmd, PACKET_SIZE, 0, (struct sockaddr*)&addr_drone, sizeof(addr_drone));

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
	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;
	return ret;
}

int init_navdata_ack()
{
	int ret;
	//5 to reset navdata mask
	//Send ACK_CONTROL_MODE
	char * ctrl_cmd[] = {"5","0"};
	if (set_cmd(HEAD_CTRL, ctrl_cmd, 2) < 0)
		return -1;
	ret = send_cmd();
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

	//bind du socket client pour le forcer sur le port choisi
	if (bind(sock_cmd, (struct sockaddr*)&addr_client, sizeof(addr_client)) < 0) {
		fprintf(stderr, "[~] Can't bind socket to port %d\n", PORT_CMD);
		return -1;
	}

	//réinitialiser les commandes
	pthread_mutex_lock(&mutex_cmd);
	cmd_no_sq = 1;
	cmd_current = NULL;
	pthread_mutex_unlock(&mutex_cmd);

	//com_master doesn't need to be initialized anymore.
/*	if (!jakopter_com_master_is_init())
		jakopter_com_init_master(NB_CHANNELS);
*/
	pthread_mutex_lock(&mutex_stopped);
	stopped = 0;
	pthread_mutex_unlock(&mutex_stopped);

	int navdata_status = navdata_connect();
	if (navdata_status == -1) {
		perror("[~] Navdata connection failed");
		return -1;
	}


	//démarrer le thread
	if (pthread_create(&cmd_thread, NULL, cmd_routine, NULL) < 0) {
		perror("[~] Can't create thread");
		return -1;
	}

	return 0;
}

int jakopter_flat_trim()
{

	if (jakopter_is_flying()) {
		fprintf(stderr, "[*] Drone is flying, setting of frame of reference canceled.\n");
		return -1;
	}
	if (set_cmd(HEAD_FTRIM, NULL, 0) < 0)
		return -1;

	nanosleep(&cmd_wait, NULL);

	printf("[*] Flat trim\n");

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return 0;
}

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

int jakopter_takeoff()
{
	if (jakopter_flat_trim() < 0) {
		fprintf(stderr, "[~] Can't establish frame of reference.\n");
		return -1;
	}

	char * args[] = {takeoff_arg};
	if (set_cmd(HEAD_REF, args, 1) < 0)
		return -1;


	//set timeout
	int no_sq;
	no_sq = navdata_no_sq();
	//Attente depart
	while(no_sq == 0) {
		nanosleep(&cmd_wait, NULL);
		no_sq = navdata_no_sq();
		printf("[*] Waiting 0\n");
	}
	while(!jakopter_is_flying() || jakopter_height() < 500) {
		nanosleep(&cmd_wait, NULL);
		no_sq = navdata_no_sq();
		printf("[*] Waiting height\n");
	}

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;


	return 0;
}

int jakopter_land()
{
	pthread_mutex_lock(&mutex_stopped);
	if (!cmd_no_sq || stopped) {
		pthread_mutex_unlock(&mutex_stopped);

		fprintf(stderr, "[~] Communication isn't initialized\n");
		return -1;
	}
	pthread_mutex_unlock(&mutex_stopped);

	char * args[] = {land_arg};
	if (set_cmd(HEAD_REF, args, 1) < 0)
		return -1;

	int init_no_sq = navdata_no_sq();
	int no_sq;
	int i = 0;

	while (no_sq == 0 && i < NAVDATA_ATTEMPT) {
		nanosleep(&cmd_wait, NULL);
		no_sq = navdata_no_sq();
		i++;
	}


	init_no_sq = navdata_no_sq();
	int emergency = 0;
	i = 0;
	while (jakopter_is_flying() && jakopter_height() > 500 && !emergency) {
		nanosleep(&cmd_wait, NULL);
		no_sq = navdata_no_sq();

		//emergency if no new data received
		if (no_sq == init_no_sq && i >= NAVDATA_ATTEMPT)
			emergency = jakopter_emergency();

		i++;
	}


	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

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

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return 0;
}

int jakopter_rotate_left()
{
	//-0.8
	char * args[] = {"1","0","0","0","-1085485875"};
	if (set_cmd(HEAD_PCMD, args, 5) < 0)
		return -1;

	//Condition navdata
	printf("Yaw : %f", jakopter_y_axis());
	nanosleep(&cmd_wait, NULL);
	printf("Yaw : %f", jakopter_y_axis());

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return 0;
}

int jakopter_rotate_right()
{
	char * args[] = {"1","0","0","0","106199773"};
	if (set_cmd(HEAD_PCMD, args, 5) < 0)
		return -1;

	nanosleep(&cmd_wait, NULL);

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return 0;
}

int jakopter_forward()
{
	char * args[] = {"1","0","-1102263091","0","0"};

	if (set_cmd(HEAD_PCMD, args, 5) < 0)
		return -1;

	nanosleep(&cmd_wait, NULL);

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return 0;
}

int jakopter_backward()
{
	char * args[] = {"1","0","104522055","0","0"};
	if (set_cmd(HEAD_PCMD, args, 5) < 0)
		return -1;

	nanosleep(&cmd_wait, NULL);

	if (set_cmd(NULL, NULL, 0) < 0)
		return -1;

	return 0;
}

int jakopter_reinit()
{
	if (set_cmd(HEAD_COM_WATCHDOG, NULL, 0) < 0)
		return -1;
	return 0;
}

int jakopter_move(float ltor, float ftob, float v_speed, float a_speed)
{
	char * args[5];
	args[0] = "1";

	char buf[SIZE_INT];
	snprintf(buf, SIZE_INT, "%d", *((int *) &ltor));
	args[1] = buf;

	char buf2[SIZE_INT];
	snprintf(buf2, SIZE_INT, "%d", *((int *) &ftob));
	args[2] = buf2;

	char buf3[SIZE_INT];
	snprintf(buf3, SIZE_INT, "%d", *((int *) &v_speed));
	args[3] = buf3;

	char buf4[SIZE_INT];
	snprintf(buf4, SIZE_INT, "%d", *((int *) &a_speed));
	args[4] = buf4;

	if (set_cmd(HEAD_PCMD, args, 5) < 0)
		return -1;

	return 0;
}

/**
 * \brief Stop main thread (End of drone connection)
*/
int jakopter_disconnect()
{
	pthread_mutex_lock(&mutex_stopped);
	if (navdata_disconnect() == 0 && !stopped) {
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

int jakopter_get_no_sq()
{
	return cmd_no_sq;
}
