#ifndef USERINPUT_H
#define USERINPUT_H

#include "common.h"
#include "com_channel.h"
#include "com_master.h"

#define USERINPUT_INTERVAL 	1/3 // interval in seconds

#define CMDFILENAME "cmd.txt"

int user_input_connect();
int user_input_disconnect();


#endif
