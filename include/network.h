#ifndef JAKOPTER_NETWORK_H
#define JAKOPTER_NETWORK_H
#include "com_channel.h"
#include "com_master.h"
#include "utils.h"

#define CHANNEL_INPUT_SIZE 16384
#define CHANNEL_OUTPUT_SIZE 1024

#define ORDER_SIZE 256

#define DEFAULT_CLIENT_IN "http://127.0.0.1"
#define DEFAULT_CLIENT_OUT "http://127.0.0.1/index.php"

/* 100 ms in ns */
#define TIMEOUT_NETWORK      10000000

int jakopter_init_network(const char* server_in, const char* server_out);
int jakopter_stop_network();
#endif