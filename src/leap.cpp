#include <iostream>
#include <string.h>
#include <math.h>
#include <Leap.h>
#include "leapdata.h"
//#include "drone.hpp"
#include "com_master.h"
#include "common.h"

using namespace Leap;

jakopter_com_channel_t* leap_channel;

class LeapListener : public Listener {
public:
	virtual void onConnect(const Controller&);
	virtual void onFrame(const Controller&);
	
private:
};

LeapListener listener;
Controller controller;

int main(int argc, char** argv) {
// 	if (jakopter_connect() < 0)
// 		return -1;
	
	controller.addListener(listener);
	
	// Keep this process running until Enter is pressed
	std::cout << "Press Enter to quit..." << std::endl;
	std::cin.get();
	
	// Remove the sample listener when done
	controller.removeListener(listener);
	
// 	if (jakopter_disconnect() < 0)
// 		return -1;	
// 	
	return 0;
}

void LeapListener::onConnect(const Controller& controller) {
	std::cout << "Connected" << std::endl;
	controller.enableGesture(Gesture::TYPE_CIRCLE);
	controller.enableGesture(Gesture::TYPE_SWIPE);	
}

void LeapListener::onFrame(const Controller& controller) {
	const Frame frame = controller.frame();
	Leap::HandList hands = frame.hands();
	Leap::Hand hand = hands[0];
	
	if(hand.isValid()){
		LeapData_t leapData;
		leapData.pitch = hand.direction().pitch();
		leapData.yaw = hand.direction().yaw();
		leapData.roll = hand.palmNormal().roll();
		leapData.height = hand.palmPosition().y;
		
		
		// 		std::cout << "Frame id: " << frame.id()
		// 		<< ", timestamp: " << frame.timestamp()
		// 		<< ", height: " << leapData.height
		// 		<< ", pitch: " << RAD_TO_DEG * leapData.pitch
		// 		<< ", yaw: " << RAD_TO_DEG * leapData.yaw
		// 		<< ", roll: " << RAD_TO_DEG * leapData.roll << std::endl;
		
// 		switch(gest.type()) {
// 			case Gesture::TYPE_CIRCLE:
// 				std::cout << "Takeoff" << std::endl;
// 				if (jakopter_takeoff() < 0)
// 					return;
// 				break;
// 			case Gesture::TYPE_SWIPE:
// 				std::cout << "Land" << std::endl;
// 				if (jakopter_land() < 0)
// 					return;
// 				break;
// 			default:
// 				break;
// 		}
		jakopter_com_write_buf(leap_channel, 0, (void*)&leapData, sizeof(leapData));
	}
}

int jakopter_connect_leap()
{
	leap_channel = jakopter_com_add_channel(CHANNEL_LEAPMOTION, sizeof(LeapData_t));
	controller.addListener(listener);
	
	return 0;
}

int jakopter_disconnect_leap()
{
	jakopter_com_destroy_channel(&leap_channel);
	controller.removeListener(listener);
	
	return 0;
}