#include "drone.h"
#include "navdata.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

/*commandes*/
navdata_t navdata_cmd;

/*Thread qui se charge d'envoyer régulièrement des commandes pour rester co avec le drone*/
pthread_t navdata_thread;
bool stopped_navdata = true;      //Guard that stops any function if connection isn't initialized.

struct sockaddr_in addr_drone, addr_client;
int sock_cmd;

int recv_cmd() {
	socklen_t len = sizeof(addr_drone);
	return recvfrom(sock_cmd, &navdata_cmd, sizeof(navdata_t), 0, (struct sockaddr*)&addr_drone, &len);
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
		return 0;
	}
	
	if(select(sock_cmd+1, &fds, NULL, NULL, &timeout) <= 0) {
		perror("Pas de réponse 1er paquet ou erreur\n");
		return 0;		
	}
	
	if(recv_cmd() < 0) {
		perror("Erreur reception réponse 1er paquet\n");
		return 0;	
	}
	
	if(!(navdata_cmd.ardrone_state & (1 << 11))) {
		fprintf(stderr, "navdata_cmd.ardrone_state: %d\n", navdata_cmd.ardrone_state & (1 << 11));
		return 0;		
	}
	
	char bootstrap_cmd[] = "AT*CONFIG=\"general:navdata_demo\",\"TRUE\"\r";

	if(sendto(sock_cmd, bootstrap_cmd, strlen(bootstrap_cmd)+1, 0, (struct sockaddr*)&addr_drone, sizeof(addr_drone)) < 0) {
		perror("Erreur envoie bootstrap exit command\n");
		return 0;	
	}
 
	while(!stopped_navdata) {
		if(recv_cmd() < 0)
			perror("Erreur d'envoi au drone");
		usleep(NAVDATA_INTERVAL*1000);
	}
	
	return 0;
}

int navdata_connect(lua_State* L) {
	
	//stopper la com si elle est déjà initialisée
	if(!stopped_navdata)
		navdata_disconnect(L);
		
	addr_drone.sin_family      = AF_INET;
	addr_drone.sin_addr.s_addr = inet_addr(WIFI_ARDRONE_IP);
	addr_drone.sin_port        = htons(PORT_NAVDATA);
	
	addr_client.sin_family      = AF_INET;
	addr_client.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_client.sin_port        = htons(PORT_NAVDATA);
	
	sock_cmd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock_cmd < 0) {
		fprintf(stderr, "Erreur, impossible d'établir le socket\n");
		lua_pushnumber(L, -1);
		return 1;
	}
	
	//bind du socket client pour le forcer sur le port choisi
	if(bind(sock_cmd, (struct sockaddr*)&addr_client, sizeof(addr_client)) < 0) {
		fprintf(stderr, "Erreur : impossible de binder le socket au port %d\n", PORT_NAVDATA);
		lua_pushnumber(L, -1);
		return 1;
	}
	
	//démarrer le thread
	if(pthread_create(&navdata_thread, NULL, navdata_routine, NULL) < 0) {
		perror("Erreur création thread");
		lua_pushnumber(L, -1);
		return 1;
	}
	stopped_navdata = false;
	
	lua_pushnumber(L, 0);
	return 1;
}


int navdata_disconnect(lua_State* L) {
	if(!stopped_navdata) {
		stopped_navdata = true;
		close(sock_cmd);
		lua_pushnumber(L, pthread_join(navdata_thread, NULL));
	}
	else {
		fprintf(stderr, "Erreur : la communication est déjà stoppée\n");
		lua_pushnumber(L, -1);
	}
	return 1;
}
