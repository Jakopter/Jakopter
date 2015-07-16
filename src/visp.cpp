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
static vpDisplayX *display_X = NULL;
static vpDisplayOpenCV *display_cv = NULL;
static bool init_track = false;

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
		if (display_X)
			delete display_X;
		if (display_cv)
			delete display_cv;
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
		if (display_X)
			delete display_X;
		if (display_cv)
			delete display_cv;
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
