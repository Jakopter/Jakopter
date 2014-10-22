#include "navdata.h"



/*commandes*/
navdata_t navdata_cmd;


/*Thread qui se charge d'envoyer régulièrement des commandes pour rester co avec le drone*/
pthread_t navdata_thread;
bool stopped_navdata = true;      //Guard that stops any function if connection isn't initialized.
static pthread_mutex_t mutex_navdata = PTHREAD_MUTEX_INITIALIZER;

struct sockaddr_in addr_drone, addr_client;
int sock_cmd;

int recv_cmd() {

	pthread_mutex_lock(&mutex_navdata);
	socklen_t len = sizeof(addr_drone);
	int ret = recvfrom(sock_cmd, &navdata_cmd, sizeof(navdata_t), 0, (struct sockaddr*)&addr_drone, &len);
	pthread_mutex_unlock(&mutex_navdata);
	return ret;
}

/*Fonction de navdata_thread*/
void* navdata_routine(void* args) {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(sock_cmd, &fds);
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	if(sendto(sock_cmd, "\x01", 1, 0, (struct sockaddr*)&addr_drone, sizeof(addr_drone)) < 0) {
		perror("Erreur d'envoi 1er paquet navdata\n");
		pthread_exit(NULL);
	}

	if(select(sock_cmd+1, &fds, NULL, NULL, &timeout) <= 0) {
		perror("Pas de réponse 1er paquet ou erreur\n");
		pthread_exit(NULL);
	}

	if(recv_cmd() < 0) {
		perror("Erreur reception réponse 1er paquet\n");
		pthread_exit(NULL);
	}

	if(!(navdata_cmd.ardrone_state & (1 << 11))) {
		fprintf(stderr, "navdata_cmd.ardrone_state: %d\n", navdata_cmd.ardrone_state & (1 << 11));
		pthread_exit(NULL);
	}
	
	char bootstrap_cmd[] = "AT*CONFIG=\"general:navdata_demo\",\"TRUE\"\r";

	if(sendto(sock_cmd, bootstrap_cmd, strlen(bootstrap_cmd)+1, 0, (struct sockaddr*)&addr_drone, sizeof(addr_drone)) < 0) {
		perror("Erreur envoie bootstrap exit command\n");
		pthread_exit(NULL);
	}

	while(!stopped_navdata) {
		if(recv_cmd() < 0)
			perror("Erreur d'envoi au drone");
		usleep(NAVDATA_INTERVAL*1000);
	}

	pthread_exit(NULL);
}

int navdata_connect() {

	//stopper la com si elle est déjà initialisée
	if(!stopped_navdata)
		navdata_disconnect();

	addr_drone.sin_family      = AF_INET;
	addr_drone.sin_addr.s_addr = inet_addr(WIFI_ARDRONE_IP);
	addr_drone.sin_port        = htons(PORT_NAVDATA);

	addr_client.sin_family      = AF_INET;
	addr_client.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_client.sin_port        = htons(PORT_NAVDATA);

	sock_cmd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock_cmd < 0) {
		fprintf(stderr, "Erreur, impossible d'établir le socket\n");
		return -1;
	}

	//bind du socket client pour le forcer sur le port choisi
	if(bind(sock_cmd, (struct sockaddr*)&addr_client, sizeof(addr_client)) < 0) {
		fprintf(stderr, "Erreur : impossible de binder le socket au port %d\n", PORT_NAVDATA);
		return -1;
	}

	stopped_navdata = false;
	//démarrer le thread
	if(pthread_create(&navdata_thread, NULL, navdata_routine, NULL) < 0) {
		perror("Erreur création thread");
		return -1;
	}

	return 0;
}


int navdata_disconnect() {
	if(!stopped_navdata) {
		stopped_navdata = true;
		close(sock_cmd);
		return pthread_join(navdata_thread, NULL);
	}

	fprintf(stderr, "Erreur : la communication est déjà stoppée\n");
	return -1;
}
