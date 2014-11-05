#include "video_decode.h"

static AVCodec* codec;
static AVCodecContext* context;
static AVPacket video_packet;
static AVFrame* current_pic;

/*Load up the h264 codec needed for video decoding.
Perform the initialization steps required by FFmpeg.*/
int video_init_decoder() {

	//try to load h264
	codec = avcodec_find_decoder(CODEC_ID_H264);
	if(!codec) {
		fprintf(stderr, "FFmpeg error : Counldn't find needed codec H264 for video decoding.\n");
		return -1;
	}
	//itilialize the ffmpeg codec context
	context = avcodec_alloc_context();
	if(avcodec_open(context, codec) < 0) {
		fprintf(stderr, "FFmpeg error : Couldn't open codec.\n");
		return -1;
	}
	//initialize the video packet and picture structures
	av_init_packet(&video_packet);
	current_pic = avcodec_alloc_frame();
	return 0;
}
/*
Decode a video buffer.
Returns:
	0 : buffer decoded, but no image produce (incomplete).
	> 0 : decoded n images.
	-1 : error while decoding.
*/
int video_decode_packet(char* buffer, int buf_size) {
	//number of bytes processed by the decoder
	int decodedLen = 0;
	//do we have a whole picture ?
	int complete_pic = 0;
	//how many pictures have we decoded ?
	int nb_pic = 0;

	if(buf_size <= 0 || buffer == NULL)
		return 0;
	
	video_packet.size = buf_size;
	video_packet.data = buffer;
	
	//send the packet's data to the decoder until it's completely processed.
	while(video_packet.size > 0) {
		//1. feed the decoder our data.
		decodedLen = avcodec_decode_video2(context, current_pic, &complete_pic, &video_packet);
		if(decodedLen < 0) {
			fprintf(stderr, "Error : couldn't decode frame.\n");
			return -1;
		}
		//2. did we get a new complete picture ?
		if(complete_pic)
			nb_pic++;
		
		//3. modify our packet's data offset to reflect the decoder's progression
		video_packet.size -= decodedLen;
		video_packet.data += decodedLen;
	}
	
	return nb_pic;
}

void video_close_decoder() {
	avcodec_close(context);
	av_free(context);
	av_free(picture);
}
