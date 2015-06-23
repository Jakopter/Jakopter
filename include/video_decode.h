#ifndef JAKOPTER_VIDEO_DECODE_H
#define JAKOPTER_VIDEO_DECODE_H

#include <libavcodec/avcodec.h>
#include "video.h"

/*The actual size of the buffer that will receive video data through TCP.
Now that we use a frame parser before decoding, we don't need to pad it.*/
#define TCP_VIDEO_BUF_SIZE BASE_VIDEO_BUF_SIZE

/*Deal with libavcodec API changes*/
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
	#define av_frame_alloc	avcodec_alloc_frame
	#define av_frame_unref	avcodec_get_frame_defaults
	#define av_frame_free	avcodec_free_frame
#endif

/*Our desired log level for libavcodec. It will be set at init.*/
#define JAKO_FFMPEG_LOG AV_LOG_PANIC

/*Load up the h264 codec needed for video decoding.
Perform the initialization steps required by FFmpeg.*/
int video_init_decoder();

/**
  * \brief Decode a video buffer.
  * \return
  * 0 : buffer decoded, but no image produce (incomplete).
  * > 0 : decoded n images.
  * -1 : error while decoding.
*/
int video_decode_packet(uint8_t* buffer, int buf_size, jakopter_video_frame_t* result);

/** \brief Free the decoder and its associated structures.*/
void video_stop_decoder();

#endif
