#ifndef JAKOPTER_USERINPUT_H
#define JAKOPTER_USERINPUT_H

#include "com_channel.h"
#include "com_master.h"
#include "utils.h"

#define USERINPUT_INTERVAL 	1/3 // interval in seconds

#define CMDFILENAME "/tmp/jakopter_cmd.txt"

int user_input_connect();
int user_input_disconnect();


#endif
