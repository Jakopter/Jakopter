#include "Client.h"
#include <iostream>
#include <string>
#include <cstdlib>
extern "C" {
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
}

#define COORDS_FILENAME "/tmp/jakopter_coords.txt"

namespace Vicon = ViconDataStreamSDK::CPP;
namespace Direction = Vicon::Direction;
namespace Result = Vicon::Result;

Vicon::Client* sdk;

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
		std::string name = sdk->GetSubjectName(0).SubjectName;

		unsigned int segments = sdk->GetSegmentCount(name).SegmentCount;

		unsigned int ForcePlateCount = sdk->GetForcePlateCount().ForcePlateCount;
		std::cout << "Force Plates: " << ForcePlateCount << std::endl;

		for (unsigned int i = 0; i < ForcePlateCount; ++i) {
			double *center = sdk->GetGlobalCentreOfPressure(i).CentreOfPressure;
			std::cout << "Center of pressure of #"<< i <<" : " << center[0] << " " << center[1] << " " << center[3] << std::endl;
		}

		if (!name.empty()) {
			std::string segment = sdk->GetSegmentName(name, 0).SegmentName;

			if (!segment.empty()) {
				std::cout << "Writing global translation" << std::endl;
				FILE *coords;
				coords = fopen(COORDS_FILENAME, "w");
				fprintf(coords, "%f ", sdk->GetSegmentGlobalTranslation(name, segment).Translation[0]);
				fprintf(coords, "%f ", sdk->GetSegmentGlobalTranslation(name, segment).Translation[1]);
				fprintf(coords, "%f ", sdk->GetSegmentGlobalTranslation(name, segment).Translation[2]);
				fclose(coords);
			}
		}
		usleep(2000);
	}

	pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
	sdk = new Vicon::Client();

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


	sdk->EnableSegmentData();
	sdk->EnableMarkerData();
	sdk->EnableUnlabeledMarkerData();
	sdk->EnableDeviceData();

	sdk->SetStreamMode(Vicon::StreamMode::ClientPull);

	//Z-Up
	sdk->SetAxisMapping(Direction::Forward, Direction::Left, Direction::Up);

	initialized = true;
	if (pthread_create(&vicon_thread, NULL, vicon_routine, NULL) < 0) {
		perror("Can't create thread");
		return -1;
	}

	std::cout << "Press Enter to quit..." << std::endl;
	std::cin.get();

	initialized = false;
	pthread_join(vicon_thread, NULL);

	sdk->DisableSegmentData();
	sdk->DisableMarkerData();
	sdk->DisableUnlabeledMarkerData();
	sdk->DisableDeviceData();

	sdk->Disconnect();


	delete sdk;

	return 0;
}