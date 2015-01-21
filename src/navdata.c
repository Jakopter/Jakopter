#include "common.h"
#include "navdata.h"
#include "drone.h"


/*commandes*/
static union navdata_t data;



/*Thread qui se charge d'envoyer régulièrement des commandes pour rester co avec le drone*/
pthread_t navdata_thread;
bool stopped_navdata = true;      //Guard that stops any function if connection isn't initialized.
static pthread_mutex_t mutex_navdata = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_stopped = PTHREAD_MUTEX_INITIALIZER;


/*Infos réseau pour la connexion au drone*/
//adresse du drone + adresse du client (nécessaire pour forcer le n° de port)
struct sockaddr_in addr_drone_navdata, addr_client_navdata;
int sock_navdata;

int recv_cmd() {

	pthread_mutex_lock(&mutex_navdata);
	socklen_t len = sizeof(addr_drone_navdata);
	int ret = recvfrom(sock_navdata, &data, sizeof(data), 0, (struct sockaddr*)&addr_drone_navdata, &len);
	pthread_mutex_unlock(&mutex_navdata);
	return ret;
}

/*Fonction de navdata_thread*/
void* navdata_routine(void* args) {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(sock_navdata, &fds);
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	if(sendto(sock_navdata, "\x01", 1, 0, (struct sockaddr*)&addr_drone_navdata, sizeof(addr_drone_navdata)) < 0) {
		perror("Erreur d'envoi 1er paquet navdata\n");
		pthread_exit(NULL);
	}

	if(select(sock_navdata+1, &fds, NULL, NULL, &timeout) <= 0) {
		perror("Pas de réponse 1er paquet ou erreur\n");
		pthread_exit(NULL);
	}


	// boucler tant que le bootstrap n'est pas quitté
	if(recv_cmd() < 0) {
		perror("Erreur reception réponse 1er paquet\n");
		pthread_exit(NULL);
	}

	//navdata bootstrap == 1
	if(data.raw.ardrone_state & (1 << 11)) {
		fprintf(stderr, "navdata_cmd.ardrone_state navdata bootstrap: %d\n", data.raw.ardrone_state & (1 << 11));
	}

	if(data.raw.ardrone_state & (1 << 15)) {
		fprintf(stderr, "navdata_cmd.ardrone_state vbat too low: %d\n", data.raw.ardrone_state & (1 << 11));
		pthread_exit(NULL);
	}

	//boucler sur la réception du bootstrap
	if(init_navdata_bootstrap() < 0){
		fprintf(stderr, "Echec de l'envoi du bootstrap\n");
		pthread_exit(NULL);
	}

	if(recv_cmd() < 0)
		perror("Erreur d'envoi au drone");

	if(data.raw.ardrone_state & (1 << 6)) {
		fprintf(stderr, "navdata_cmd.ardrone_state control command ACK: %d\n", data.raw.ardrone_state & (1 << 6));
	}

	if(init_navdata_ack() < 0){
		fprintf(stderr, "Echec de l'acquittement de l'initialisation du navdata\n");
		pthread_exit(NULL);
	}

	pthread_mutex_lock(&mutex_stopped);
	while(!stopped_navdata) {
		pthread_mutex_unlock(&mutex_stopped);

		if(recv_cmd() < 0)
			perror("Erreur d'envoi au drone");
		usleep(NAVDATA_INTERVAL*1000);

		if(sendto(sock_navdata, "\x01", 1, 0, (struct sockaddr*)&addr_drone_navdata, sizeof(addr_drone_navdata)) < 0) {
			perror("Erreur d'envoi 1er paquet navdata\n");
			pthread_exit(NULL);
		}

		pthread_mutex_lock(&mutex_stopped);
	}
	pthread_mutex_unlock(&mutex_stopped);

	pthread_exit(NULL);
}

int navdata_connect() {

	//stopper la com si elle est déjà initialisée
	if(!stopped_navdata)
		navdata_disconnect();

	addr_drone_navdata.sin_family      = AF_INET;
	addr_drone_navdata.sin_addr.s_addr = inet_addr(WIFI_ARDRONE_IP);
	addr_drone_navdata.sin_port        = htons(PORT_NAVDATA);

	addr_client_navdata.sin_family      = AF_INET;
	addr_client_navdata.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_client_navdata.sin_port        = htons(PORT_NAVDATA);

	sock_navdata = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock_navdata < 0) {
		fprintf(stderr, "Erreur, impossible d'établir le socket\n");
		return -1;
	}

	//bind du socket client pour le forcer sur le port choisi
	if(bind(sock_navdata, (struct sockaddr*)&addr_client_navdata, sizeof(addr_client_navdata)) < 0) {
		fprintf(stderr, "Erreur : impossible de binder le socket au port %d\n", PORT_NAVDATA);
		return -1;
	}

	pthread_mutex_lock(&mutex_stopped);
	stopped_navdata = false;
	pthread_mutex_unlock(&mutex_stopped);

	//démarrer le thread
	if(pthread_create(&navdata_thread, NULL, navdata_routine, NULL) < 0) {
		perror("Erreur création thread");
		return -1;
	}

	return 0;
}


int jakopter_is_flying() {
	int flyState = -1;
	pthread_mutex_lock(&mutex_navdata);
	flyState = data.raw.ardrone_state & 0x0001;
	pthread_mutex_unlock(&mutex_navdata);
	return flyState;
}



int jakopter_height() {
	int height = -1;
	if(data.raw.options[0].tag != TAG_DEMO){
		perror("Le tag actuel ne correspond pas au TAG_DEMO.");
		return height;
	}
	pthread_mutex_lock(&mutex_navdata);
	printf("Header: %x\n",data.demo.header);
	printf("Masque: %x\n",data.demo.ardrone_state);
	printf("Sequence num: %d\n",data.demo.sequence);
	printf("Tag: %x\n",data.demo.tag);
	printf("Size: %d\n",data.demo.size);
	printf("Fly state: %x\n",data.demo.ctrl_state); //Masque défini dans ctrl_states.h
	printf("Theta: %f\n",data.demo.theta);
	printf("Phi: %f\n",data.demo.phi);
	printf("Psi: %f\n",data.demo.psi);
	height = data.demo.altitude;
	pthread_mutex_unlock(&mutex_navdata);
	//Nombre de valeurs retournées
	return height;
}


int navdata_disconnect() {
	pthread_mutex_lock(&mutex_stopped);
	if(!stopped_navdata) {
		stopped_navdata = true;
		pthread_mutex_unlock(&mutex_stopped);
		int ret = pthread_join(navdata_thread, NULL);

		close(sock_navdata);

		return ret;
	}
	else {
		pthread_mutex_unlock(&mutex_stopped);

		fprintf(stderr, "Erreur : la communication est déjà stoppée\n");
		return -1;
	}

}
