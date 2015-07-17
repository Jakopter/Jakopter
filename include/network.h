/* Jakopter
 * Copyright © 2015 ALF@INRIA
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */
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
/** \brief Start the thread that listen on server_in and send data on server_out
  * \param server_in a HTTP address where Curl GET his data
  * \param server_out a HTTP address where Curl POST his data
  * \return 0 OK, -1 otherwise
  */
int jakopter_init_network(const char* server_in, const char* server_out);
/** \brief Stop curl */
int jakopter_stop_network();
#endif