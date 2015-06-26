#include <visp/vpDisplayOpenCV.h>
#include <visp/vpDisplayX.h>
#include <visp/vpDetectorFace.h>
#include <visp/vpDetectorQRCode.h>
#include <visp/vpDot2.h>

#include "visp.h"


static bool initialized = false;
static vpImage<unsigned char>* display_img = NULL;
static vpDetectorFace face_detector;
#ifdef VISP_HAVE_ZBAR
static vpDetectorQRCode qr_detector;
static vpImage<unsigned char>* qr_img = NULL;
#endif
static vpDot2 blob;
static vpImagePoint germ;
static vpDisplayX* display_X = NULL;
static bool init_track = false;

/*
* channels for data input and output.
* Input expects navdata in the following order :
* battery%, altitude, angles (3), speed (3)
* Output returns face detection data:
* size, x of the center of the face, y of the center of the face
*/
static jakopter_com_channel_t *com_in;
static jakopter_com_channel_t *com_out;
/* Last saved modification timestamp from the input channel */
static double prev_update = 0;

int visp_init()
{
	com_in = jakopter_com_add_channel(CHANNEL_DISPLAY, VISP_COM_IN_SIZE);
	if (com_in == NULL) {
		fprintf(stderr, "[~][Display] Couldn't create com channel in.\n");
		return -1;
	}
	com_out = jakopter_com_add_channel(CHANNEL_CALLBACK, VISP_COM_OUT_SIZE);
	if (com_out == NULL) {
		fprintf(stderr, "[~][Display] Couldn't create com channel out.\n");
		return -1;
	}
	jakopter_com_write_float(com_out, 0, 0.0);
	jakopter_com_write_float(com_out, 4, 0.0);
	jakopter_com_write_float(com_out, 8, 0.0);
	prev_update = 0;

	return 0;
}

void visp_destroy()
{
	if (initialized) {
		delete display_X;
		delete display_img;
	}
	jakopter_com_remove_channel(CHANNEL_DISPLAY);
	jakopter_com_remove_channel(CHANNEL_CALLBACK);
}
/*
int blob_process(uint8_t* frame, int width, int height, int size) {

	try {
		if (!initialized) {
			display_img = new vpImage<unsigned char>(height, width);
		#if defined(VISP_HAVE_X11)
			display_X = new vpDisplayX(*display_img);
		#endif
			vpDisplay::setTitle(*display_img, "Video reader");

			blob.setGraphics(true);
			blob.setGraphicsThickness(2);

			initialized = true;
		}
		if (initialized) {
			//In order to use color, RGBa
			vpImageConvert::YUV420ToGrey((unsigned char *)frame, display_img->bitmap, width*height);

			vpDisplay::display(*display_img);
			//compute blob
			if (!init_track) {
				blob.initTracking(*display_img, germ);
				init_track = true;
			}
			else {
				blob.track(*display_img);
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
*/
int qrcode_process(uint8_t* frame, int width, int height, int size) {
	try {
		if (!initialized) {
			qr_img = new vpImage<unsigned char>(height, width);
		#if defined(VISP_HAVE_X11)
			display_X = new vpDisplayX(*qr_img);
		#endif
			vpDisplay::setTitle(*qr_img, "Video reader");

			initialized = true;
		}
		if (initialized) {
			//In order to use color, RGBa
			vpImageConvert::YUV420ToGrey((unsigned char *)frame, qr_img->bitmap, width*height);

			vpDisplay::display(*qr_img);

			//compute qr_code
			bool status = qr_detector.detect(*qr_img);
			int obj = qr_detector.getNbObjects();

			if (status) {
				for(size_t i = 0; i < obj; i++) {
					std::vector<vpImagePoint> p = qr_detector.getPolygon(i);
					vpRect bbox = qr_detector.getBBox(i);
					vpDisplay::displayRectangle(*qr_img, bbox, vpColor::green);
					vpDisplay::displayText(*qr_img, (int)(bbox.getTop()-10), (int)bbox.getLeft(),
						"Message: \"" + qr_detector.getMessage(i) + "\"",
					vpColor::red);
					for(size_t j = 0; j < p.size(); j++) {
						vpDisplay::displayCross(*qr_img, p[j], 14, vpColor::red, 3);
						std::ostringstream number;
						number << j;
						vpDisplay::displayText(*qr_img, p[j]+vpImagePoint(10,0), number.str(), vpColor::blue);
					}
					double x = 0;
					double y = 0;
					bbox.getCenter(x, y);

					jakopter_com_write_float(com_out, 0, (float)bbox.getSize());
					jakopter_com_write_float(com_out, 4, (float)x);
					jakopter_com_write_float(com_out, 8, (float)y);
				}
			}


			int bat = jakopter_com_read_int(com_in, 0);
			std::ostringstream bat_str;
			bat_str << bat;
			vpDisplay::displayText(*qr_img, (int)qr_img->getHeight()-25, 10, "Battery : " + bat_str.str() + " %", vpColor::red);
			vpDisplay::flush(*qr_img);
		}
	}
	catch (vpException e) {
		std::cout << "Catch an exception: " << e << std::endl;
	}
	return 0;
}

int visp_process(uint8_t* frame, int width, int height, int size)
{
	//return qrcode_process(frame, width, height, size);
	//alt_tree, lbpcascade
	std::string face_cascade_script = "/usr/share/opencv/lbpcascades/lbpcascade_frontalface.xml";

	try {
		if (!initialized) {
			display_img = new vpImage<unsigned char>(height, width);
		#if defined(VISP_HAVE_X11)
			display_X = new vpDisplayX(*display_img);
		#endif
			vpDisplay::setTitle(*display_img, "Video reader");

    		face_detector.setCascadeClassifierFile(face_cascade_script);

			initialized = true;
		}
		if (initialized) {
			//In order to use color, RGBa
			vpImageConvert::YUV420ToGrey((unsigned char *)frame, display_img->bitmap, width*height);
			vpDisplay::display(*display_img);
			//compute face detection
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

					double x = 0;
					double y = 0;
					bbox.getCenter(x, y);

					jakopter_com_write_float(com_out, 0, (float)bbox.getSize());
					jakopter_com_write_float(com_out, 4, (float)x);
					jakopter_com_write_float(com_out, 8, (float)y);
				}
			}
			int bat = jakopter_com_read_int(com_in, 0);
			std::ostringstream bat_str;
			bat_str << bat;
			vpDisplay::displayText(*display_img, (int)display_img->getHeight()-25, 10, "Battery : " + bat_str.str() + " %", vpColor::red);
			vpDisplay::flush(*display_img);
		}
	}
	catch (vpException e) {
		std::cout << "Catch an exception: " << e << std::endl;
	}
	return 0;
}