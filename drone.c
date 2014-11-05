#include "drone.h"
#include "navdata.h"
#include "video.h"

/*commandes pour décoller et atterrir*/
char ref_cmd[PACKET_SIZE];
char *ref_head = "AT*REF",
	 *takeoff_arg="290718208",
	 *land_arg="290717696";

/*N° de commande actuel*/
int cmd_no_sq = 0;
/*commande en cours d'envoi, et ses arguments*/
char *cmd_current = NULL, *cmd_current_args = NULL;

/*Thread qui se charge d'envoyer régulièrement des commandes pour rester co avec le drone*/
pthread_t cmd_thread;
int stopped = 1;      //Guard that stops any function if connection isn't initialized.
static pthread_mutex_t mutex_cmd = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_stopped = PTHREAD_MUTEX_INITIALIZER;




/* Change la commande courante : prend le nom de la commande
et une chaîne avec les arguments
 * \param cmd_type header de la forme AT*SOMETHING
 * \param args code commande à transmettre (takeoff_arg)
*/
void set_cmd(char* cmd_type, char* args) {
	pthread_mutex_lock(&mutex_cmd);
	cmd_current = cmd_type;
	cmd_current_args = args;
	pthread_mutex_unlock(&mutex_cmd);
}

/*Envoie la commande courante, et incrémente le compteur*/
int send_cmd() {
	pthread_mutex_lock(&mutex_cmd);
	if(cmd_current != NULL) {
		memset(ref_cmd, 0, PACKET_SIZE);
		snprintf(ref_cmd, PACKET_SIZE, "%s=%d,%s\r", cmd_current, cmd_no_sq, cmd_current_args);
		cmd_no_sq++;
		ref_cmd[PACKET_SIZE-1] = '\0';

		int ret = sendto(sock_cmd, ref_cmd, PACKET_SIZE, 0, (struct sockaddr*)&addr_drone, sizeof(addr_drone));
		pthread_mutex_unlock(&mutex_cmd);

		return ret;
	}

	pthread_mutex_unlock(&mutex_cmd);

	return 0;
}

/*Fonction de cmd_thread*/
void* cmd_routine(void* args) {
	struct timespec itv = {0, TIMEOUT_CMD};
	pthread_mutex_lock(&mutex_stopped);
	while(!stopped) {
		pthread_mutex_unlock(&mutex_stopped);

		if(send_cmd() < 0)
			perror("Erreur d'envoi au drone");
		nanosleep(&itv, NULL);

		pthread_mutex_lock(&mutex_stopped);
	}
	pthread_mutex_unlock(&mutex_stopped);

	pthread_exit(NULL);
}

/*créer un socket + initialiser l'adresse du drone et du client ; remettre le nb de commandes à 1.
Démarrer le thread de commande.
Appelle stop si le thread est déjà en train de tourner.*/
int jakopter_connect(lua_State* L) {

	//stopper la com si elle est déjà initialisée
	pthread_mutex_lock(&mutex_stopped);
	if(!stopped) {
		pthread_mutex_unlock(&mutex_stopped);
		jakopter_disconnect(L);
		
	}
	else
		pthread_mutex_unlock(&mutex_stopped);

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
	pthread_mutex_lock(&mutex_cmd);
	cmd_no_sq = 1;
	cmd_current = NULL;
	cmd_current_args = NULL;
	pthread_mutex_unlock(&mutex_cmd);

	pthread_mutex_lock(&mutex_stopped);
	stopped = 0;
	pthread_mutex_unlock(&mutex_stopped);

	navdata_connect();

	//démarrer le thread
	if(pthread_create(&cmd_thread, NULL, cmd_routine, NULL) < 0) {
		perror("Erreur création thread");
		lua_pushnumber(L, -1);
		return 1;
	}

	lua_pushnumber(L, 0);
	return 1;
}

/*faire décoller le drone (échoue si pas init).*/
int jakopter_takeoff(lua_State* L) {

	//vérifier qu'on a initialisé
	pthread_mutex_lock(&mutex_stopped);
	if(!cmd_no_sq || stopped) {
		pthread_mutex_unlock(&mutex_stopped);
		fprintf(stderr, "Erreur : la communication avec le drone n'a pas été initialisée\n");
		lua_pushnumber(L, -1);
		return 1;
	}
	else
		pthread_mutex_unlock(&mutex_stopped);

	//changer la commande
	//takeoff = 0x11540200, land = 0x11540000

	set_cmd(ref_head, takeoff_arg);
	lua_pushnumber(L, 0);
	return 1;
}

/*faire atterrir le drone*/
int jakopter_land(lua_State* L) {
	//vérifier qu'on a initialisé
	pthread_mutex_lock(&mutex_stopped);
	if(!cmd_no_sq || stopped) {
		pthread_mutex_unlock(&mutex_stopped);

		fprintf(stderr, "Erreur : la communication avec le drone n'a pas été initialisée\n");
		lua_pushnumber(L, -1);
		return 1;
	}
	else
		pthread_mutex_unlock(&mutex_stopped);

	set_cmd(ref_head, land_arg);
	lua_pushnumber(L, 0);
	return 1;
}

/*Arrêter le thread principal (fin de la co au drone)*/
int jakopter_disconnect(lua_State* L) {
	pthread_mutex_lock(&mutex_stopped);
	if(!stopped) {
		stopped = 1;
		pthread_mutex_unlock(&mutex_stopped);

		close(sock_cmd);
		navdata_disconnect();
		lua_pushnumber(L, pthread_join(cmd_thread, NULL));
	}
	else {
		pthread_mutex_unlock(&mutex_stopped);
		
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
	{"connect_video", jakopter_init_video},
	{"stop_video", jakopter_stop_video},
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
