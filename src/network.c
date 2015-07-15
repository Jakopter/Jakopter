#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <curl/curl.h>
#include <curl/easy.h>

#include "network.h"

//Input: data received in Jakopter in JSON
//Output: data emitted outside by POST method
jakopter_com_channel_t* network_in_channel;
jakopter_com_channel_t* network_out_channel;

pthread_t send_thread;
pthread_t receive_thread;
/*Guard that stops any function if connection isn't initialized.*/
bool stopped_network = true;
//boolean to know if the thread started
bool recv_ready = false;
struct CurlData {
	char *memory;
	size_t size;
};

static char send_addr[256];
static char recv_addr[256];


void* send_routine(void* args)
{
	//char* url = "http://127.0.0.1/index.php";
	CURL* send_handle = curl_easy_init();
	//struct curl_slist *header = NULL;
	CURLcode ret;
	struct timespec itv = {0, TIMEOUT_NETWORK};

	char buf[CHANNEL_OUTPUT_SIZE];

	if (!send_handle) {
		perror("[~][network] Can't create curl handle for send");
		return NULL;
	}
	while (!stopped_network) {
		char order[ORDER_SIZE];
		size_t order_len = (uint)jakopter_com_read_int(network_out_channel,4);
		memset(order, 0, ORDER_SIZE);
		memset(buf, 0, CHANNEL_OUTPUT_SIZE);

		if (order_len > ORDER_SIZE) {
			fprintf(stderr, "[~][network] size of order > 256\n");
			continue;
		}

		size_t offset = 8 + ORDER_SIZE;
		snprintf(buf, CHANNEL_OUTPUT_SIZE,
			//"id=%d&order=%s&t_x=%f&t_y=%f&t_z=%f&r_x=%f&r_y=%f&r_z=%f&m=%f,%f,%f,%f,%f,%f,%f,%f,%f",
			"id=%d&order=%s&t_x=%f&t_y=%f&t_z=%f&r_x=%f&r_y=%f&r_z=%f",
			jakopter_com_read_int(network_out_channel, 0),
			(char*)jakopter_com_read_buf(network_out_channel, 8, order_len, &order),
			jakopter_com_read_float(network_out_channel, offset),
			jakopter_com_read_float(network_out_channel, offset+4),
			jakopter_com_read_float(network_out_channel, offset+8),
			jakopter_com_read_float(network_out_channel, offset+12),
			jakopter_com_read_float(network_out_channel, offset+16),
			jakopter_com_read_float(network_out_channel, offset+20)
			// jakopter_com_read_float(network_out_channel, offset+24),
			// jakopter_com_read_float(network_out_channel, offset+28),
			// jakopter_com_read_float(network_out_channel, offset+32),
			// jakopter_com_read_float(network_out_channel, offset+36),
			// jakopter_com_read_float(network_out_channel, offset+40),
			// jakopter_com_read_float(network_out_channel, offset+44),
			// jakopter_com_read_float(network_out_channel, offset+48),
			// jakopter_com_read_float(network_out_channel, offset+52),
			// jakopter_com_read_float(network_out_channel, offset+56)
			);
		curl_easy_setopt(send_handle, CURLOPT_POSTFIELDS, buf);
		curl_easy_setopt(send_handle, CURLOPT_VERBOSE, 0L);
		curl_easy_setopt(send_handle, CURLOPT_URL, (char*)args);
		curl_easy_setopt(send_handle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

		ret = curl_easy_perform(send_handle);

		if (ret != CURLE_OK) {
			fprintf(stderr, "[~][network] curl_easy_perform(send) failed: %s\n", curl_easy_strerror(ret));
			break;
		}

		nanosleep(&itv, NULL);
		itv.tv_nsec = TIMEOUT_NETWORK;
	}
	curl_easy_cleanup(send_handle);

	return NULL;
}

static size_t write_com_channel(void* data, size_t size, size_t nmemb, void* userptr)
{
	size_t realsize = size * nmemb;
	//printf("[*][network] Write %d of %lu : %s \n", (int)realsize, nmemb, (char*)data);
	jakopter_com_write_int(network_in_channel, 0, (int)realsize);
	jakopter_com_write_buf(network_in_channel, 4, data, realsize);
	memset(data, 0, realsize);
	return realsize;
}
void* receive_routine(void* args)
{
	CURL* recv_handle = curl_easy_init();
	CURLcode ret;
	struct timespec itv = {1, TIMEOUT_NETWORK/10};
	//CURLINFO info;
	if (!recv_handle) {
		perror("[~][network] Can't create curl handle for receive");
		return NULL;
	}
	struct CurlData serv_data;
	while (!stopped_network) {
		curl_easy_setopt(recv_handle, CURLOPT_WRITEFUNCTION, write_com_channel);
		curl_easy_setopt(recv_handle, CURLOPT_WRITEDATA, (void *)&serv_data);
		curl_easy_setopt(recv_handle, CURLOPT_VERBOSE, 0L);
		curl_easy_setopt(recv_handle, CURLOPT_URL, (char*)args);
		curl_easy_setopt(recv_handle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
		ret = curl_easy_perform(recv_handle);

		if (ret != CURLE_OK) {
			fprintf(stderr, "[~][network] curl_easy_perform(receive) failed: %s\n", curl_easy_strerror(ret));
			break;
		}

		nanosleep(&itv, NULL);
		itv.tv_nsec = TIMEOUT_NETWORK;

		recv_ready = true;
	}
	curl_easy_cleanup(recv_handle);

	return NULL;
}

int jakopter_init_network(const char* server_in, const char* server_out)
{
	if (!stopped_network)
		return -1;
	stopped_network = false;
	memset(recv_addr, 0, 256);
	memset(send_addr, 0, 256);

	if (server_out && server_out[0] != '\0')
		strncpy(send_addr, server_out, strlen(server_out));
	else
		strncpy(send_addr, DEFAULT_CLIENT_OUT, strlen(DEFAULT_CLIENT_OUT));

	if (server_in && server_in[0] != '\0')
		strncpy(recv_addr, server_in, strlen(server_in));
	else
		strncpy(recv_addr, DEFAULT_CLIENT_IN, strlen(DEFAULT_CLIENT_IN));

	printf("[*][network] Receiving from: %s\n", recv_addr);
	printf("[*][network] Sending to: %s\n", send_addr);

	curl_global_init(CURL_GLOBAL_ALL);

	network_in_channel  = jakopter_com_add_channel(CHANNEL_NETWORK_INPUT, CHANNEL_INPUT_SIZE);
	network_out_channel = jakopter_com_add_channel(CHANNEL_NETWORK_OUTPUT, CHANNEL_OUTPUT_SIZE);

	printf("[network] channel created\n");

	if (pthread_create(&send_thread, NULL, send_routine, (void*) send_addr) < 0) {
		perror("[~][network] Can't create thread send");
		return -1;
	}
	if (pthread_create(&receive_thread, NULL, receive_routine, (void*) recv_addr) < 0) {
		perror("[~][network] Can't create thread send");
		return -1;
	}

	int i = 0;
	while (!recv_ready && i < 200) {
		usleep(500);
		i++;
	}

	printf("[network] threads created\n");

	return -(i >= 200);
}

int jakopter_stop_network()
{
	if (!stopped_network) {
		stopped_network = true;

		int ret_out = pthread_join(send_thread, NULL);
		int ret_in = pthread_join(receive_thread, NULL);
		recv_ready = false;
		jakopter_com_remove_channel(CHANNEL_NETWORK_INPUT);
		jakopter_com_remove_channel(CHANNEL_NETWORK_OUTPUT);

		curl_global_cleanup();

		int ret = 0;
		if (ret_in)
			ret |= 2;
		if (ret_out)
			ret |= 1;
		return -ret;
	}
	else {
		fprintf(stderr, "[*][network] Network already stopped\n");
		return -1;
	}
	//thread join
	return 0;
}