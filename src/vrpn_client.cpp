/* Jakopter
 * Copyright © 2015 ALF@INRIA
 * Author : Alexandre Leonardi@Immersia
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
#include <iostream>
#include <string>
#include "vrpn_Analog.h"
#include "vrpn_Button.h"

#define CMDFILENAME "/tmp/jakopter_cmd.txt"

using namespace std;

/*Default values of analog channels and buttons to use, set for an usage with Myo Armband*/
static int pitch_channel = 1;
static int roll_channel = 0;
static int yaw_channel = 2;
/* fist */
static int up_button = 2;
/* spread fingers */
static int down_button = 5;
/* double tap */
static int land_button = 6;

/** \brief Called each time there is a new update coming from the VRPN server concerning datas from analog channels
 *	\param userData useless
 *	\param a The datas sent by the VRPN device
 */
void VRPN_CALLBACK handle_analog(void* userData, const vrpn_ANALOGCB a)
{
	//char to write in the cmd file
	char c = 's';

	float pitch = a.channel[pitch_channel];
	float yaw = a.channel[yaw_channel];
	float roll = a.channel[roll_channel];
	cout << pitch << " ";
	cout << roll << " ";
	cout << yaw << endl;

	if (roll > 0.7)
		c = 'l';
	else if (roll < -0.7)
		c = 'r';

	if (pitch > 0.7)
		c = 'b';
	else if (pitch < -0.7)
		c = 'f';

	FILE *cmd = fopen(CMDFILENAME,"w");
	fprintf(cmd, "%c\n", c);
	fclose(cmd);
}

/** \brief Called each time there is a new update coming from the VRPN server concerning datas from buttons
 *	\param userData useless
 *	\param b The datas sent by the VRPN device
 */
void VRPN_CALLBACK handle_button(void* userData, const vrpn_BUTTONCB b)
{
	char c = 's';

	if(b.button == up_button && b.state == 1)
		c = 'u';//up
	if(b.button == down_button && b.state == 1)
		c = 'd';//down
	if(b.button == land_button && b.state == 1)
		c = 'k';//land

	cout << c << endl;

	FILE *cmd = fopen(CMDFILENAME,"w");
	fprintf(cmd, "%c\n", c);
	fclose(cmd);
}

/** \brief Checks if a given string is part of a given array of char*, starting at position 1 (and NOT 0)
 *	\param option The string to search
 *	\param argv The array to search in
 *	\param argc The size of the array
 *	\return option's position if there is a match ; 0 otherwise
 */
int argv_containing(string option, char** argv,	int argc)
{
	int i = 0;
	for (i = 1; i < argc; i++) {
		if(option.compare(argv[i]) == 0)
			break;
	}
	return i;
}

/** \brief Displays help for the usage of vrpn_client, called when the option --help is used
 */
void display_help()
{
	string help;

	help = "Usage : ./vrpn_client VRPN_DEVICE_NAME@VRPN_SERVER_ADRESS [OPTION]\n";
	help += "Connects to a VR device through a VRPN server, allowing you to control the drone using any device supported by VRPN.\n";
	help += "The VRPN_DEVICE_NAME is defined in the .cfg file located in the same folder as the VRPN server.";
	help += "The VRPN_SERVER_ADRESS can either be an IP adress or the computer on which the server is running's name.\n";
	help += "\t e.g. Mouse0@localhost or Mouse0@127.0.0.1\n";

	help += "\nEach option has a mandatory argument following it, which has to be a number\n";
	help += "\t-pitch\t\t\tspecifies that you want to set a custom value for the analog channel bind to the drone's pitch\n";
	help += "\t-roll\t\t\tspecifies that you want to set a custom value for the analog channel bind to the drone's roll\n";
	help += "\t-yaw\t\t\tspecifies that you want to set a custom value for the analog channel bind to the drone's pitch\n";
	help += "\t-up\t\t\tspecifies that you want to set a custom button to make the drone go up or take off\n";
	help += "\t-down\t\t\tspecifies that you want to set a custom button to make the drone go down\n";
	help += "\t-land\t\t\tspecifies that you want to set a custom button to make the drone land\n";

	cout << help << endl;
}

/** \brief Main program to support any device recognized by a VRPN server.
 *	\param argc Number of execution parameters
 *	\param argv[] Array containing the execution parameters :
 *		(mandatory) Name of the device and VRPN server's IP adress, e.g. Mouse0@127.0.0.1 or Mouse0@localhost,	must always be in first position
 *		"-pitch" signals that the following paramter is an int which is the analog channel to use for the pitch of the drone
 *		"-roll" signals that the following paramter is an int which is the analog channel to use for the roll of the drone
 *		"-yaw" signals that the following paramter is an int which is the analog channel to use for the yaw of the drone
 *		"-up" signals that the following paramter is an int which is the button to use to take off and go up
 *		"-down" signals that the following paramter is an int which is the button to use to go down
 *		"-land" signals that the following paramter is an int which is the button to use to land
 * \return 0 in case of success, -1 in case of error or help requested
 */
int main(int argc, char* argv[])
{
	//if there is only 1 argument in argv, that means the name and adress of the devices weren't specidifed
	if (argc == 1) {
		cout << "[vrpn_client.cpp/main] ERROR : you have to specify the tracker's name and vrpn server's IP adress.\n
				Use ./vrpn_client --help for help (makes sense right ?)" << endl;
		return -1;
	}

	//if the user has called ./vrpn_client --help (eventually followed by others options, which will be ignored), display the help
	if (argv_containing("--help", argv, argc)) {
		display_help();
		return -1;
	}

	//the name of the device to track, and the adress of the VRPN server, are always the first argument and are of the form Mouse0@127.0.0.1 or Mouse0@localhost
	char* tracker = argv[1];

	//for each of the possible options of the program, we check if the user used them, and if appropriate, we modify the default values of the analog channels/buttons
	int index = argv_containing("-pitch", argv, argc);
	pitch_channel = index
			? std::stoi(*(new string(argv[index+1])))
			: pitch_channel;

	index = argv_containing("-roll", argv, argc);
	roll_channel = index
			? std::stoi(*(new string(argv[index+1])))
			: roll_channel;

	index = argv_containing("-yaw", argv, argc);
	yaw_channel = index
			? std::stoi(*(new string(argv[index+1])))
			: yaw_channel;

 	index = argv_containing("-up", argv, argc);
	up_channel = index
			? std::stoi(*(new string(argv[index+1])))
			: up_channel;

 	index = argv_containing("-down", argv, argc);
	down_channel = index
			? std::stoi(*(new string(argv[index+1])))
			: down_channel;

 	index = argv_containing("-land", argv, argc);
	land_channel = index
			? std::stoi(*(new string(argv[index+1])))
			: land_channel;

	vrpn_Analog_Remote* vrpnAnalog = new vrpn_Analog_Remote(tracker);
	vrpnAnalog->register_change_handler(0, handle_analog);

	vrpn_Button_Remote* vrpnButton = new vrpn_Button_Remote(tracker);
	vrpnButton->register_change_handler(0, handle_button);

	while(1) {
		vrpnAnalog->mainloop();
		vrpnButton->mainloop();
	}

	delete vrpnAnalog;
	delete vrpnButton;

	return 0;
}
