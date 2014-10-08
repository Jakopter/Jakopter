#include "lauxlib.h"
#include "lua.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PACKET_SIZE      256
#define PORT_CMD         5556
#define PORT_NAVDATA     5554
#define TIMEOUT_CMD      30000
#define TIMEOUT_NAVDATA  30000
#define WIFI_ARDRONE_IP "192.168.1.1"

/*commandes pour décoller et aterrir*/
char ref_cmd[PACKET_SIZE];
char *ref_head = "AT*REF", 
	 *takeoff_arg="290718208", 
	 *land_arg="290717696";

char *config_head = "AT*CONFIG", 
	 *bootstrap_mode_arg="\"general:navdata_demo\",\"TRUE\"";

/*N° de commande actuel*/
int cmd_no_sq = 0;
/*commande en cours d'envoi, et ses arguments*/
char *cmd_current = NULL, *cmd_current_args = NULL;

/*Thread qui se charge d'envoyer régulièrement des commandes pour rester co avec le drone*/
pthread_t cmd_thread;
int stopped = 1;      //Guard that stops any function if connection isn't initialized.

/*Infos réseau pour la connexion au drone*/
//adresse du drone + adresse du client (nécessaire pour forcer le n° de port)
struct sockaddr_in addr_drone, addr_client;
int sock_cmd;

/*Déclarer quelques fonctions à l'avance*/
int send_cmd();
int luaopen_drone(lua_State* L);
int stop(lua_State* L);


/*Fonction de cmd_thread*/
void* cmd_routine(void* args) {
	struct timespec itv = {0, TIMEOUT_CMD};
	while(!stopped) {
		if(send_cmd() < 0)
			perror("Erreur d'envoi au drone");
		nanosleep(&itv, NULL);
	}
	
	pthread_exit(NULL);
}


/*Fonction de navdata_thread*/
void* navdata_routine(void* args) {
	struct timespec itv = {0, TIMEOUT_NAVDATA};
	while(!stopped) {
		if(send_cmd() < 0)
			perror("Erreur d'envoi au drone");
		nanosleep(&itv, NULL);
	}
	
	pthread_exit(NULL);
}

/*créer un socket + initialiser l'adresse du drone et du client ; remettre le nb de commandes à 1.
Démarrer le thread de commande.
Appelle stop si le thread est déjà en train de tourner.*/
int jakopter_connect(lua_State* L) {
	
	//stopper la com si elle est déjà initialisée
	if(!stopped)
		stop(L);
		
	addr_drone.sin_family      = AF_INET;
	addr_drone.sin_addr.s_addr = inet_addr(WIFI_ARDRONE_IP);
	addr_drone.sin_port        = htons(PORT_CMD);
	
	addr_client.sin_family      = AF_INET;
	addr_client.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_client.sin_port        = htons(PORT_CMD);
	
	sock_cmd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock_cmd < 0) {
		fprintf(stderr, "Erreur, impossible d'établir le socket\n");
		lua_pushnumber(L, -1);
		return 1;
	}
	
	//bind du socket client pour le forcer sur le port choisi
	if(bind(sock_cmd, (struct sockaddr*)&addr_client, sizeof(addr_client)) < 0) {
		fprintf(stderr, "Erreur : impossible de binder le socket au port %d\n", PORT_CMD);
		lua_pushnumber(L, -1);
		return 1;
	}
	
	//réinitialiser les commandes
	cmd_no_sq = 1;
	cmd_current = NULL;
	cmd_current_args = NULL;
	
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
void set_cmd(char* cmd_type, char* args) {
	
	cmd_current = cmd_type;
	cmd_current_args = args;
}

/*Envoie la commande courante, et incrémente le compteur*/
int send_cmd() {
	if(cmd_current != NULL) {
		memset(ref_cmd, 0, PACKET_SIZE);
		snprintf(ref_cmd, PACKET_SIZE, "%s=%d,%s\r", cmd_current, cmd_no_sq, cmd_current_args);
		cmd_no_sq++;
		ref_cmd[PACKET_SIZE-1] = '\0';

		return sendto(sock_cmd, ref_cmd, PACKET_SIZE, 0, (struct sockaddr*)&addr_drone, sizeof(addr_drone));
	}
	return 0;
}

/*faire décoller le drone (échoue si pas init).*/
int jakopter_takeoff(lua_State* L) {
	
	//vérifier qu'on a initialisé
	if(!cmd_no_sq || stopped) {
		fprintf(stderr, "Erreur : la communication avec le drone n'a pas été initialisée\n");
		lua_pushnumber(L, -1);
		return 1;
	}
	
	//changer la commande
	//takeoff = 0x11540200, land = 0x11540000
	
	set_cmd(ref_head, takeoff_arg);
	lua_pushnumber(L, 0);
	return 1;
}

/*faire atterrir le drone*/
int jakopter_land(lua_State* L) {
	//vérifier qu'on a initialisé
	if(!cmd_no_sq || stopped) {
		fprintf(stderr, "Erreur : la communication avec le drone n'a pas été initialisée\n");
		lua_pushnumber(L, -1);
		return 1;
	}
	
	set_cmd(ref_head, land_arg);
	lua_pushnumber(L, 0);
	return 1;
}

/*Arrêter le thread principal (fin de la co au drone)*/
int jakopter_disconnect(lua_State* L) {
	if(!stopped) {
		stopped = 1;
		close(sock_cmd);
		lua_pushnumber(L, pthread_join(cmd_thread, NULL));
	}
	else {
		fprintf(stderr, "Erreur : la communication est déjà stoppée\n");
		lua_pushnumber(L, -1);
	}
	return 1;
}


/*Obtenir le nombre actuel de commandes*/
int jakopter_get_no_sq(lua_State* L) {
	lua_pushnumber(L, cmd_no_sq);
	return 1;
}

//enregistrer les fonctions pour lua
//ou luaL_reg
static const luaL_Reg jakopterlib[] = {
	{"connect", jakopter_connect},
	{"takeoff", jakopter_takeoff},
	{"land", jakopter_land},
	{"disconnect", jakopter_disconnect},
	{"get_no_sq", jakopter_get_no_sq},
	{NULL, NULL}
};

int luaopen_drone(lua_State* L) {
	//lua 5.1 et 5.2 incompatibles...
#if LUA_VERSION_NUM <= 501
	luaL_register(L, "jakopter", jakopterlib);
#else
	lua_newtable(L);
	luaL_setfuncs(L, jakopterlib, 0);
#endif
	return 1;
}