#include <visp/vpDisplayOpenCV.h>
#include <visp/vpDisplayX.h>
#include <visp/vpDetectorFace.h>
#include <visp/vpVideoReader.h>
#include "visp.h"


static bool initialized = false;
static vpImage<unsigned char>* display_img = NULL;
static vpDetectorFace face_detector;
static vpDisplayX* display_X = NULL;

/*
* channels for data input and output.
* Input expects navdata in the following order :
* battery%, altitude, angles (3), speed (3)
*/
static jakopter_com_channel_t *com_in;
/* Last saved modification timestamp from the input channel */
static double prev_update = 0;

int visp_init()
{
	com_in = jakopter_com_add_channel(CHANNEL_DISPLAY, DISPLAY_COM_IN_SIZE);
	if (com_in == NULL) {
		fprintf(stderr, "[~][Display] Couldn't create com channel.\n");
		return -1;
	}
	prev_update = 0;

	return 0;
}

void visp_destroy()
{
	if(initialized) {
		delete display_X;
		delete display_img;
	}
	jakopter_com_remove_channel(CHANNEL_DISPLAY);
}

int visp_process(uint8_t* frame, int width, int height, int size)
{
	std::string opt_face_cascade_name;
	opt_face_cascade_name = "/usr/share/opencv/haarcascades/haarcascade_frontalface_alt.xml";

	try {
		if (!initialized) {
			display_img = new vpImage<unsigned char>(height, width);
		#if defined(VISP_HAVE_X11)
			display_X = new vpDisplayX(*display_img);
		#endif
			vpDisplay::setTitle(*display_img, "Video reader");

    		face_detector.setCascadeClassifierFile(opt_face_cascade_name);

			initialized = true;
		}
		if (initialized) {

			// unsigned char* rgb_frame = (unsigned char*)malloc(width*height*sizeof(unsigned char));

			vpImageConvert::YUV420ToGrey((unsigned char *)frame, display_img->bitmap, width*height);

			// memcpy(display_img->bitmap, rgb_frame, display_img->getNumberOfPixel());

			// free(rgb_frame);

			vpDisplay::display(*display_img);
			//traitement
			bool face_found = face_detector.detect(*display_img);
			if (face_found) {
				std::ostringstream text;
				text << "Found " << face_detector.getNbObjects() << " face(s)";
				vpDisplay::displayText(*display_img, 10, 10, text.str(), vpColor::red);
				for (size_t i = 0 ; i < face_detector.getNbObjects() ; i++) {
					std::vector<vpImagePoint> p = face_detector.getPolygon(i);
					vpRect bbox = face_detector.getBBox(i);
					vpDisplay::displayRectangle(*display_img, bbox, vpColor::green, false, 4);
					vpDisplay::displayText(*display_img, (int)bbox.getTop()-10, (int)bbox.getLeft(), "Message: \"" + face_detector.getMessage(i) + "\"", vpColor::red);
				}
			}
			int bat = jakopter_com_read_int(com_in, 0);
			std::ostringstream bat_str;
			bat_str << bat;
			vpDisplay::displayText(*display_img, (int)display_img->getHeight()-25, 10, "Battery : " + bat_str.str() + " %%", vpColor::red);
			vpDisplay::flush(*display_img);
		}
	}
	catch (vpException e) {
		std::cout << "Catch an exception: " << e << std::endl;
	}
	return 0;
}
