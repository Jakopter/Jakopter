#define LUA_LIB
#define PACKET_SIZE 256

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "lua.h"
#include "lauxlib.h"

/*commandes pour décoller et aterrir*/
char cmd[PACKET_SIZE];
char* cmdhead = "AT*REF", *takeoff_arg="290718208", *land_arg="290717696";

/*N° de commande actuel*/
int cmd_no_sq = 0;
/*commande en cours d'envoi, et ses arguments*/
char* cmd_now = NULL, *cmd_now_args = NULL;

/*Thread qui se charge d'envoyer régulièrement des commandes pour rester co avec le drone*/
pthread_t cmd_thread;
int stopped = 1;

/*Infos réseau pour la connexion au drone*/
//adresse du drone + adresse du client (nécessaire pour forcer le n° de port)
struct sockaddr_in addr_drone, addr_client;
int sock, port = 5556;
char* drone_ip = "192.168.1.1";

/*Déclarer quelques fonctions à l'avance*/
int sendcmd();
int luaopen_drone(lua_State* L);
int stop(lua_State* L);


/*Fonction de cmd_thread*/
void* cmd_routine(void* args) {
	struct timespec itv = {0, 30000};
	while(!stopped) {
		if(sendcmd() < 0)
			perror("Erreur d'envoi au drone");
		nanosleep(&itv, NULL);
	}
	
	pthread_exit(NULL);
}

/*créer un socket + initialiser l'adresse du drone et du client ; remettre le nb de commandes à 1.
Démarrer le thread de commande.
Appelle stop si le thread est déjà en train de tourner.*/
int init_connection(lua_State* L) {
	
	//stopper la com si elle est déjà initialisée
	if(!stopped)
		stop(L);
		
	addr_drone.sin_family = AF_INET;
	addr_drone.sin_addr.s_addr=inet_addr(drone_ip);
	addr_drone.sin_port = htons(port);
	
	addr_client.sin_family = AF_INET;
	addr_client.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_client.sin_port = htons(port);
	
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock < 0){
		fprintf(stderr, "Erreur, impossible d'établir le socket\n");
		lua_pushnumber(L, -1);
		return 1;
	}
	
	//bind du socket client pour le forcer sur le port choisi
	if(bind(sock, (struct sockaddr*)&addr_client, sizeof(addr_client)) < 0) {
		fprintf(stderr, "Erreur : impossible de binder le socket au port %d\n", port);
		lua_pushnumber(L, -1);
		return 1;
	}
	
	//réinitialiser les commandes
	cmd_no_sq = 1;
	cmd_now = NULL;
	cmd_now_args = NULL;
	
	//démarrer le thread
	if(pthread_create(&cmd_thread, NULL, cmd_routine, NULL) < 0) {
		 perror("Erreur création thread");
		lua_pushnumber(L, -1);
		return 1;
	}
	stopped = 0;
	
	lua_pushnumber(L, 0);
	return 1;
}


/*Change la commande courante : prend le nom de la commande
et une chaîne avec les arguments*/
void setcmd(char* cmd_type, char* args) {
	
	cmd_now = cmd_type;
	cmd_now_args = args;
}

/*Envoie la commande courante, et incrémente le compteur*/
int sendcmd() {
	if(cmd_now != NULL) {
		
		memset(cmd, 0, PACKET_SIZE);
		snprintf(cmd, PACKET_SIZE, "%s=%d,%s\r", cmd_now, cmd_no_sq++, cmd_now_args);
		cmd[PACKET_SIZE-1] = '\0';

		return sendto(sock, cmd, PACKET_SIZE, 0, (struct sockaddr*)&addr_drone, sizeof(addr_drone));
	}
	return 0;
}

/*faire décoller le drone (échoue si pas init).*/
int takeoff(lua_State* L) {
	
	//vérifier qu'on a initialisé
	if(!cmd_no_sq || stopped) {
		fprintf(stderr, "Erreur : la communication avec le drone n'a pas été initialisée\n");
		lua_pushnumber(L, 0);
		return 1;
	}
	
	//changer la commande
	//takeoff = 0x11540200, land = 0x11540000
	
	setcmd(cmdhead, takeoff_arg);
	lua_pushnumber(L, 0);
	return 1;
}

/*faire aterrir le drone*/
int land(lua_State* L) {
	//vérifier qu'on a initialisé
	if(!cmd_no_sq || stopped) {
		fprintf(stderr, "Erreur : la communication avec le drone n'a pas été initialisée\n");
		lua_pushnumber(L, 0);
		return 1;
	}
	
	setcmd(cmdhead, land_arg);
	lua_pushnumber(L, 0);
	return 1;
}

/*Arrêter le thread principal (fin de la co au drone)*/
int stop(lua_State* L) {
	if(!stopped) {
		stopped = 1;
		close(sock);
		lua_pushnumber(L, pthread_join(cmd_thread, NULL));
	}
	else {
		fprintf(stderr, "Erreur : la communication est déjà stoppée\n");
		lua_pushnumber(L, -1);
	}
	return 1;
}

/*Obtenir le nombre actuel de commandes*/
int get_cmd_count(lua_State* L) {
	lua_pushnumber(L, cmd_no_sq);
	return 1;
}

//enregistrer les fonctions pour lua
//ou luaL_reg
static const luaL_Reg dronelib[] = {

	{"init", init_connection},
	{"takeoff", takeoff},
	{"land", land},
	{"stop", stop},
	{"get_cmd_count", get_cmd_count},

	{NULL, NULL}
};

int luaopen_drone(lua_State* L) {
	//lua 5.1 et 5.2 incompatibles...
#if LUA_VERSION_NUM <= 501
	luaL_register(L, "drone", dronelib);
#else
	lua_newtable(L);
	luaL_setfuncs(L, dronelib, 0);
#endif
	return 1;
}

