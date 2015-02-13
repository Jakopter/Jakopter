#include <iostream>
#include <string.h>
#include <math.h>
#include <Leap.h>
#include "leapdata.h"

extern "C" {
	#include "com_master.h"
	#include "common.h"
}

using namespace Leap;

jakopter_com_channel_t* leap_channel = NULL;

class LeapListener : public Listener {
public:
	virtual void onConnect(const Controller&);
	virtual void onFrame(const Controller&);
	
private:
};

LeapListener listener;
Controller controller;

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
		if(leap_channel)
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
	if(leap_channel)
		jakopter_com_destroy_channel(&leap_channel);
	controller.removeListener(listener);
	
	return 0;
}