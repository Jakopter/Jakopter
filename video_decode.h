#include <libavcodec/avcodec.h>
#include "video.h"

#define TCP_VIDEO_BUF_SIZE BASE_VIDEO_BUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE

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
int video_decode_packet(char* buffer, int buf_size);

/*Free the decoder.*/
void video_close_decoder();
