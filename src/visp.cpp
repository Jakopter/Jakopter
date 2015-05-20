#include <visp/vpDisplayOpenCV.h>
#include <visp/vpDisplayX.h>
#include <visp/vpDetectorFace.h>
#include <visp/vpVideoReader.h>


std::string opt_face_cascade_name;
std::string opt_video;

int visp_init()
{
	return 0;
}

int visp_destroy()
{
	return 0;
}

int visp_process(uint8_t* frame, int width, int height, int size)
{
	try {
		opt_face_cascade_name = "/usr/share/opencv/haarcascades/haarcascade_frontalface_alt.xml";
		//input
		opt_video = "video.mpeg";
		unsigned char * rgb_frame;

		// vpImageConvert::::YUV420ToRGB((unsigned char *)frame,rgb_frame, width, height)
		vpImage<unsigned char> I;
		vpFrameGrabber *g;

#if defined(VISP_HAVE_X11)
		vpDisplayX d(I);
#endif

		g->open(I);// Open the framegrabber
		g->acquire(I);

		vpDisplay::setTitle(I, "Video reader");

		vpDisplay::display(I);
		vpDisplay::flush(I);
	}
	catch (vpException e) {
		std::cout << "Catch an exception: " << e << std::endl;
	}
	return 0;
}
