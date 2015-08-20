/* Jakopter
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
#include <visp/vpDisplayOpenCV.h>
#include <visp/vpDisplayX.h>
#include <visp/vpDetectorFace.h>
#include <visp/vpDetectorQRCode.h>
#include <visp/vpDot2.h>

#include "visp.h"


/** Set when display is ready */
static bool initialized = false;
/** Set to load process settings when switching */
static bool process_changed = false;
static vpImage<vpRGBa> *display_img = NULL;
static vpImage<unsigned char> *grey_img = NULL;
static vpDetectorFace face_detector;
#ifdef VISP_HAVE_ZBAR
static vpDetectorQRCode qr_detector;
#endif
static vpDot2 blob;
static vpImagePoint germ;
#if defined(VISP_HAVE_X11)
static vpDisplayX *display_X = NULL;
#elif defined(VISP_HAVE_OPENCV)
static vpDisplayOpenCV *display_cv = NULL;
#endif
static bool init_track = false;

/******** Screenshot options ********/
/* Set to 1 if the user wants to capture a screenshot; then set back to 0 automatically */
static int want_screenshot = 0;
/* Total number of screenshots taken, used for screenshot filenames */
static int screenshot_nb = 0;
/* Base screenshot name. Final name = base name + screenshot_nb */
static std::string screenshot_baseName = "screen_visp_";
/*
* Take a screenshot, store it in a file named according to the total screenshot count.
*/
static void take_screenshot(uint8_t* frame, int size);

/*
* channels for data input and output.
* Input expects navdata in the following order :
* battery%, altitude, angles (3), speed (3)
* Output returns for face detection data:
* size, x of the center of the face, y of the center of the face
* Output returns qrcode detection data:
* size, x of the center of the qr, y of the center of the qr, the message encoded in the qrcode
*/
static jakopter_com_channel_t *com_in;
static jakopter_com_channel_t *com_out;
/* Last saved modification timestamp from the input channel */
static double prev_update = 0;

static int face_process(uint8_t* frame, int width, int height, int size);
static pthread_mutex_t mutex_process = PTHREAD_MUTEX_INITIALIZER;
/** Callback chooser */
int (*current_process)(uint8_t*, int, int, int) = face_process;

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
#if defined(VISP_HAVE_X11)
		if (display_X)
			delete display_X;
#elif defined(VISP_HAVE_OPENCV)
		if (display_cv)
			delete display_cv;
#endif
		if (display_img)
			delete display_img;
		if (grey_img)
			delete grey_img;
		initialized = false;
	}
	jakopter_com_remove_channel(CHANNEL_DISPLAY);
	jakopter_com_remove_channel(CHANNEL_CALLBACK);
}

static void init_display(int width, int height)
{
	//exit if initialized and resolution is the same
	if (initialized) {
		uint display_h = display_img->getHeight();
		uint display_w = display_img->getWidth();
		if ((uint)height == display_h && (uint)width == display_w)
			return;

		//replace vpImages and display if resolution changed
#if defined(VISP_HAVE_X11)
		if (display_X)
			delete display_X;
#elif defined(VISP_HAVE_OPENCV)
		if (display_cv)
			delete display_cv;
#endif
		delete display_img;
		delete grey_img;
	}

	display_img = new vpImage<vpRGBa>(height, width);
	grey_img = new vpImage<unsigned char>(height, width);
#if defined(VISP_HAVE_X11)
	display_X = new vpDisplayX(*display_img);
#elif defined(VISP_HAVE_OPENCV)
	display_cv = new vpDisplayOpenCV(*display_img);
#endif

	vpDisplay::setTitle(*display_img, VIDEO_TITLE);
}

static void prepare_display(uint8_t* frame, int width, int height, int size)
{
	//Grey image often required by detectors (face and qr need it)
	vpImageConvert::YUV420ToGrey((unsigned char *)frame, grey_img->bitmap, width*height);

	//In order to use color, convert vpRGBa from a buffer of unsigned char in RGBa (4 values per pixel)
	unsigned char* buf = (unsigned char*)malloc(4*size*sizeof(unsigned char));
	vpImageConvert::YUV420ToRGBa((unsigned char *)frame, buf, width, height);
	memcpy(display_img->bitmap, buf, 4*width*height);
	free(buf);

	vpDisplay::display(*display_img);
}

static int blob_process(uint8_t* frame, int width, int height, int size)
{
	init_display(width, height);

	if (!initialized || process_changed) {
		blob.setGraphics(true);
		blob.setGraphicsThickness(2);
		initialized = true;
		process_changed = false;
	}

	prepare_display(frame, width, height, size);

	//compute blob
	if (!init_track) {
		blob.initTracking(*grey_img, germ);
		init_track = true;
	}
	else {
		blob.track(*grey_img);
	}

	int bat = jakopter_com_read_int(com_in, 0);
	std::ostringstream bat_str;
	bat_str << bat;
	vpDisplay::displayText(*display_img, (int)display_img->getHeight()-25, 10,
		"Battery : " + bat_str.str() + " %%", vpColor::red);

	vpDisplay::flush(*display_img);
	return 0;
}

static int qrcode_process(uint8_t* frame, int width, int height, int size)
{
	init_display(width, height);

	if (!initialized  || process_changed) {
		initialized = true;
		process_changed = false;
	}

	prepare_display(frame, width, height, size);

	if (qr_detector.detect(*grey_img)) {
		std::vector<vpImagePoint> pol = qr_detector.getPolygon(0);
		vpRect bbox = qr_detector.getBBox(0);
		vpDisplay::displayRectangle(*display_img, bbox, vpColor::green);
		vpDisplay::displayText(*display_img, (int)(bbox.getTop()-10), (int)bbox.getLeft(),
			"Message: \"" + qr_detector.getMessage(0) + "\"",
			vpColor::red);

		for (size_t j = 0; j < pol.size(); j++) {
			vpDisplay::displayCross(*display_img, pol[j], 14, vpColor::red, 3);
			std::ostringstream number;
			number << j;
			vpDisplay::displayText(*display_img, pol[j]+vpImagePoint(10,0),
				number.str(), vpColor::blue);
		}
		double x = 0;
		double y = 0;
		bbox.getCenter(x, y);

		std::string msg = qr_detector.getMessage(0);
		char cstr[SIZE_MESSAGE];
		strncpy(cstr, msg.c_str(),SIZE_MESSAGE);

		jakopter_com_write_float(com_out, 0, (float)bbox.getSize());
		jakopter_com_write_float(com_out, 4, (float)x);
		jakopter_com_write_float(com_out, 8, (float)y);
		jakopter_com_write_int(com_out, 12, strlen(cstr)+1);
		jakopter_com_write_buf(com_out, 16, cstr, SIZE_MESSAGE);
	}

	int bat = jakopter_com_read_int(com_in, 0);
	std::ostringstream bat_str;
	bat_str << bat;
	vpDisplay::displayText(*display_img, (int)display_img->getHeight()-25, 10,
		"Battery : " + bat_str.str() + " %", vpColor::red);

	vpDisplay::flush(*display_img);
	return 0;
}

static int face_process(uint8_t* frame, int width, int height, int size)
{
	//alt_tree, lbpcascade
	std::string face_cascade_script = "/usr/share/opencv/lbpcascades/lbpcascade_frontalface.xml";

	init_display(width, height);

	if (!initialized  || process_changed) {
		face_detector.setCascadeClassifierFile(face_cascade_script);
		initialized = true;
		process_changed = false;
	}

	prepare_display(frame, width, height, size);

	//compute face detection
	bool face_found = face_detector.detect(*grey_img);
	if (face_found) {
		std::ostringstream text;
		text << "Found " << face_detector.getNbObjects() << " face(s)";
		vpDisplay::displayText(*display_img, 10, 10, text.str(), vpColor::red);
		//we get the first face in the list, which is also the biggest
		std::vector<vpImagePoint> p = face_detector.getPolygon(0);
		vpRect bbox = face_detector.getBBox(0);
		vpDisplay::displayRectangle(*display_img, bbox, vpColor::green, false, 4);
		vpDisplay::displayText(*display_img, (int)bbox.getTop()-10, (int)bbox.getLeft(),
			"Message: \"" + face_detector.getMessage(0) + "\"",
			vpColor::red);

		double x = 0;
		double y = 0;
		bbox.getCenter(x, y);

		jakopter_com_write_float(com_out, 0, (float)bbox.getSize());
		jakopter_com_write_float(com_out, 4, (float)x);
		jakopter_com_write_float(com_out, 8, (float)y);
	}
	int bat = jakopter_com_read_int(com_in, 0);
	std::ostringstream bat_str;
	bat_str << bat;
	vpDisplay::displayText(*display_img, (int)display_img->getHeight()-25, 10,
		"Battery : " + bat_str.str() + " %", vpColor::red);

	vpDisplay::flush(*display_img);

	return 0;
}

int visp_process(uint8_t* frame, int width, int height, int size)
{
	int ret = -1;
	try {
		pthread_mutex_lock(&mutex_process);
		ret = current_process(frame, width, height, size);
		pthread_mutex_unlock(&mutex_process);
	}
	catch (vpException e) {
		std::cout << "Catch an exception: " << e << std::endl;
		pthread_mutex_unlock(&mutex_process);
	}

	return ret;
}

void visp_set_process(int id)
{
	pthread_mutex_lock(&mutex_process);
	switch(id) {
	case QRCODE :
		current_process = qrcode_process;
		break;
	case BLOB :
		current_process = blob_process;
		break;
	default:
		current_process = face_process;
	}
	process_changed = true;
	pthread_mutex_unlock(&mutex_process);
}

/** \brief Save the current frame in a file called screen_%d.yuv with %d a number incremented
  * each screenshot.
  * \param frame current frame
  * \param size in bytes
  */
void take_screenshot(uint8_t* frame, int size)
{
	//get the final filename length (+1 for the \0)
	int name_length = snprintf(NULL, 0, "%s%d.yuv", screenshot_baseName.c_str(), screenshot_nb) + 1;
	char* filename = (char*)malloc(sizeof(char)*name_length);
	if (filename == NULL) {
		fprintf(stderr, "Display : couldn't allocate memory for screenshot filename\n");
		return;
	}
	snprintf(filename, name_length, "%s%d.yuv", screenshot_baseName.c_str(), screenshot_nb);
	//dump the frame to a new file, don't do any conversion for now.
	FILE* f = fopen(filename, "w");
	if (f == NULL) {
		fprintf(stderr, "Display : couldn't open file %s for writing\n", filename);
		free(filename);
		return;
	}
	fwrite(frame, sizeof(uint8_t), size/sizeof(uint8_t), f);

	fclose(f);
	printf("Display : screenshot taken, saved to %s\n", filename);
	free(filename);
	screenshot_nb++;
}