#ifndef JAKOPTER_VIDEO_OPENCV_H
#define JAKOPTER_VIDEO_OPENCV_H

int video_oc_init();
void video_oc_destroy();
int video_oc_process(uint8_t*, int, int, int);

#endif
