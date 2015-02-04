#ifndef JAKOPTER_COMMON_H
#define JAKOPTER_COMMON_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#define float32_t float
#define float64_t double

#define WIFI_ARDRONE_IP	"192.168.1.1"

//init dans drone commande
struct sockaddr_in addr_drone, addr_client;
int sock_cmd;

#endif
