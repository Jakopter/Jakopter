#include "video_decode.h"

static AVCodec* codec;
static AVCodecContext* context;
static AVPacket video_packet;
static AVFrame* current_frame;

/*Load up the h264 codec needed for video decoding.
Perform the initialization steps required by FFmpeg.*/
int video_init_decoder() {

	//initialize libavcodec
	avcodec_register_all();
	//try to load h264
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if(codec == NULL)
        codec = avcodec_find_decoder(CODEC_ID_H264);
	if(codec == NULL) {
		fprintf(stderr, "FFmpeg error : Counldn't find needed codec H264 for video decoding.\n");
		return -1;
	}

	//itilialize the ffmpeg codec context
	context = avcodec_alloc_context3(codec);
	if(avcodec_open2(context, codec, NULL) < 0) {
		fprintf(stderr, "FFmpeg error : Couldn't open codec.\n");
		return -1;
	}
	//initialize the video packet and frame structures
	av_init_packet(&video_packet);
	current_frame = av_frame_alloc();
	return 0;
}
/*
Decode a video buffer.
Returns:
	0 : buffer decoded, but no image produced (incomplete).
	> 0 : decoded n images.
	-1 : error while decoding.
*/
int video_decode_packet(uint8_t* buffer, int buf_size) {
	//number of bytes processed by the decoder
	int decodedLen = 0;
	//do we have a whole frame ?
	int complete_frame = 0;
	//how many frames have we decoded ?
	int nb_frames = 0;

	if(buf_size <= 0 || buffer == NULL)
		return 0;

	video_packet.size = buf_size;
	video_packet.data = buffer;

	//send the packet's data to the decoder until it's completely processed.
	while(video_packet.size > 0) {
		//1. feed the decoder our data.
		decodedLen = avcodec_decode_video2(context, current_frame, &complete_frame, &video_packet);
		if(decodedLen < 0) {
			fprintf(stderr, "Error : couldn't decode frame.\n");
			return -1;
		}
		//2. did we get a new complete frame ?
		if(complete_frame)
			nb_frames++;

		//3. modify our packet's data offset to reflect the decoder's progression
		video_packet.size -= decodedLen;
		video_packet.data += decodedLen;
	}

	return nb_frames;
}

void video_stop_decoder() {
	avcodec_close(context);
	avcodec_free_context(&context);
	av_frame_free(&current_frame);
}
