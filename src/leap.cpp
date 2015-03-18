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
	static int64_t lastFrameID = 0;
	
	if(frame.id() < lastFrameID+10)
		return;
	
	Leap::HandList hands = frame.hands();
	Leap::Hand hand = hands[0];
	
	if(hand.isValid()){
		LeapData_t leapData;
		leapData.pitch = hand.direction().pitch();
		leapData.yaw = hand.direction().yaw();
		leapData.roll = hand.palmNormal().roll();
		leapData.height = hand.palmPosition().y;
		
//		std::cout << "Pitch: " << leapData.pitch << " Yaw: " << leapData.yaw << " Roll: " << leapData.roll << " Height: " << leapData.height << " Frame: " << frame.id() << std::endl;
		
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
	
	lastFrameID = frame.id();
}

int jakopter_connect_leap()
{
	leap_channel = jakopter_com_add_channel(CHANNEL_LEAPMOTION, sizeof(LeapData_t));
	
	// Init the channel
	LeapData_t leapData;
	leapData.pitch = 0.0f;
	leapData.yaw = 0.0f;
	leapData.roll = 0.0f;
	leapData.height = 0.0f;
	if(leap_channel)
		jakopter_com_write_buf(leap_channel, 0, (void*)&leapData, sizeof(leapData));
	
	controller.addListener(listener);
	
	return 0;
}

int jakopter_disconnect_leap()
{
	if(leap_channel)
		jakopter_com_remove_channel(CHANNEL_LEAPMOTION);
	controller.removeListener(listener);
	
	return 0;
}
