#ifndef JAKOPTER_VIDEO_DECODE_H
#define JAKOPTER_VIDEO_DECODE_H

#include <libavcodec/avcodec.h>
#include "video.h"

//The actual buffer must be a bit larger because some codecs could read over the end,
//see FFmpeg doc.
#define TCP_VIDEO_BUF_SIZE BASE_VIDEO_BUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE

//Dimensions of the video stream
#define JAKO_VIDEO_WIDTH	640
#define JAKO_VIDEO_HEIGHT	360

/*Load up the h264 codec needed for video decoding.
Perform the initialization steps required by FFmpeg.*/
int video_init_decoder();

/*
Decode a video buffer.
Returns:
	0 : buffer decoded, but no image produce (incomplete).
	> 0 : decoded n images.
	-1 : error while decoding.
*/
int video_decode_packet(uint8_t* buffer, int buf_size);

/*Free the decoder and its associated structures.*/
void video_stop_decoder();

#endif
