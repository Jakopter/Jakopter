#include <stdio.h>
#include <stdlib.h>
#include <opencv/highgui.h>
#include <opencv/cv.h>


IplImage* img = NULL;
const char* window_title = "Jakopter";

int iframe = 0;

int video_oc_init()
{

  printf("test_init\n");
  cvNamedWindow (window_title, CV_WINDOW_AUTOSIZE);
  cvWaitKey(1);

  return EXIT_SUCCESS;
}

void video_oc_destroy(){

  cvDestroyAllWindows();
  cvReleaseImage(&img);

}

int video_oc_process(uint8_t* buffer, int w, int h, int size){

  img = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 1);
  img->imageData = (char*)buffer;
  cvShowImage (window_title, img);

  /*cvCvtColor(img, img, CV_RGB2GRAY);
  CvPoint p1,p2;
  int length = 150;
  p1.x = w/2;
  p1.y = h/2;

  iframe+=1;
  //printf("test_process (frame %d)\n", iframe);

  p2.x = (100 + iframe);
  p2.y = (100 + iframe);

  CvScalar color = CV_RGB(0,0,255);

  cvCircle(img, p1, length+iframe, color, 1, 8, 0 );
  cvLine( img, p1, p2, color, 1, CV_AA, 0 );
*/

  cvWaitKey(100);

  return 0;
}
