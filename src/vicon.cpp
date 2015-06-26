#include <iostream>
#include <string>
#include <cstdlib>
#include <cfloat>
extern "C" {
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
}
#include "Client.h"

#define COORDS_FILENAME "/tmp/jakopter_coords.txt"
/* size of float digits plus 3 spaces and \0*/
#define TEXT_BUF_SIZE DECIMAL_DIG*3+4
#define VICON_DRONE_NAME "ardrone2_"

namespace Vicon = ViconDataStreamSDK::CPP;
namespace Direction = Vicon::Direction;
namespace Result = Vicon::Result;

Vicon::Client* sdk;

struct sockaddr_un addr_client_coords;
int sock_vicon;
std::string drone_id = std::string(VICON_DRONE_NAME);

bool initialized = false;
pthread_t vicon_thread;

void* vicon_routine(void* args)
{
	while (initialized) {
		std::cout << "Waiting for new frame...";
		while (sdk->GetFrame().Result != Result::Success) {
			// Sleep a little so that we don't lumber the CPU with a busy poll
			sleep(1);
			std::cout << ".";
		}
		std::cout << std::endl;

		unsigned int frame = sdk->GetSubjectCount().Result;
		if (frame == Result::NoFrame)
			std::cout << "No frame received" << std::endl;
		else
			std::cout << "Number of subjects : " << sdk->GetSubjectCount().SubjectCount << std::endl;

		unsigned int subjects = sdk->GetSubjectCount().SubjectCount;


		//Loop until it finds the drone id as defined in Vicon Tracker
		for (unsigned int i = 0 ; i < subjects ; i++) {
			std::string name = sdk->GetSubjectName(i).SubjectName;

			if (name != drone_id)
				continue;

			unsigned int segments = sdk->GetSegmentCount(name).SegmentCount;
			for (unsigned int j = 0 ; j < segments ; j++) {
				std::string segment = sdk->GetSegmentName(name, j).SegmentName;
				std::cout << "Writing global translation " << segment
						<< " for " << name << std::endl;

				float x = sdk->GetSegmentGlobalTranslation(name, segment).Translation[0];
				float y = sdk->GetSegmentGlobalTranslation(name, segment).Translation[1];
				float z = sdk->GetSegmentGlobalTranslation(name, segment).Translation[2];


				char buf [TEXT_BUF_SIZE];
				snprintf(buf, TEXT_BUF_SIZE, "%f %f %f ", x, y, z);

				std::cout << x << " " << y << " " << z << std::endl;
				sendto(sock_vicon, buf, TEXT_BUF_SIZE, 0, (struct sockaddr*)&addr_client_coords, sizeof(addr_client_coords));
			}
		}

		usleep(500*1000);
	}

	pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
	sdk = new Vicon::Client();


	if (argc == 2)
		drone_id += argv[1];

	std::cout << drone_id << std::endl;

	while(!sdk->IsConnected().Connected)
	{
		unsigned int ret = sdk->Connect("192.168.10.1:801").Result;
		if (ret != Result::Success) {
			std::cout << "Can't connect :";
			if (ret == Result::InvalidHostName)
				std::cout << "Hostname invalid" << std::endl;
			else if (ret == Result::ClientConnectionFailed)
				std::cout << "Client Connection failed" << std::endl;
		}

		std::cout << ".";
		sleep(1);
	}
	std::cout << std::endl;

	//Handle argv1=drone id in vicon
	sdk->EnableSegmentData();
	sdk->EnableMarkerData();
	sdk->EnableUnlabeledMarkerData();
	sdk->EnableDeviceData();

	sdk->SetStreamMode(Vicon::StreamMode::ClientPull);

	//Z-Up
	sdk->SetAxisMapping(Direction::Forward, Direction::Left, Direction::Up);

	memset(&addr_client_coords, '\0', sizeof(struct sockaddr_un));
	addr_client_coords.sun_family = AF_UNIX;
	strncpy(addr_client_coords.sun_path, COORDS_FILENAME, sizeof(addr_client_coords.sun_path)-1);

	sock_vicon = socket(AF_UNIX, SOCK_DGRAM, 0);

	if (sock_vicon < 0) {
		perror("[~][coords] Can't create the socket");
		return -1;
	}

	initialized = true;
	if (pthread_create(&vicon_thread, NULL, vicon_routine, NULL) < 0) {
		perror("Can't create thread");
		close(sock_vicon);
		return -1;
	}

	std::cout << "Press Enter to quit..." << std::endl;
	std::cin.get();

	initialized = false;
	pthread_join(vicon_thread, NULL);

	close(sock_vicon);

	sdk->DisableSegmentData();
	sdk->DisableMarkerData();
	sdk->DisableUnlabeledMarkerData();
	sdk->DisableDeviceData();

	sdk->Disconnect();


	delete sdk;

	return 0;
}