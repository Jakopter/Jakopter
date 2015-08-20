/* Jakopter
 * Copyright © 2014 - 2015 Thibaud Hulin, Hector Labanca, Jérémy Yziquel
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
#ifndef JAKOPTER_USERINPUT_H
#define JAKOPTER_USERINPUT_H

#include "com_channel.h"
#include "com_master.h"
#include "utils.h"

#define USERINPUT_INTERVAL 	1/3 // interval in seconds
#define USERINPUT_TIMEOUT 2000
#define USERINPUT_FILENAME "/tmp/jakopter_user_input.sock"

int user_input_connect();
int user_input_disconnect();


#endif
