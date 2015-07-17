/* Jakopter
 * Copyright © 2014 - 2015 Jérémy Yziquel, Hector Labanca
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
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <Leap.h>

#define CMDFILENAME "/tmp/jakopter_cmd.txt"

using namespace Leap;

class LeapListener : public Listener {
public:
	virtual void onConnect(const Controller&);
	virtual void onFrame(const Controller&);

private:
};

LeapListener listener;
Controller controller;

int main(int argc, char** argv) {
	controller.addListener(listener);

	// Keep this process running until Enter is pressed
	std::cout << "Press Enter to quit..." << std::endl;
	std::cin.get();

	// Remove the sample listener when done
	controller.removeListener(listener);

	return 0;
}

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

	if(hand.isValid()) {
		float pitch = hand.direction().pitch();
		float yaw = hand.direction().yaw();
		float roll = hand.palmNormal().roll();
		float height = hand.palmPosition().y;

		std::cout << "Pitch: " << RAD_TO_DEG*pitch << " Yaw: " << RAD_TO_DEG*yaw << " Roll: " << RAD_TO_DEG*roll << " Height: " << height << " Frame: " << frame.id() << std::endl;

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

		char c = 's';

		if (height > 300)
			c = 'u';
		else if(height < 75)
			c = 'k';
		else if (height < 150)
			c = 'd';


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

	lastFrameID = frame.id();
}