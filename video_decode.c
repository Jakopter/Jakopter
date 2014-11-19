#include "video_decode.h"
#include <sys/stat.h>
#include <fcntl.h>

static AVCodec* codec;
static AVCodecContext* context;
static AVCodecParserContext* cpContext;
static AVPacket video_packet;
static AVFrame* current_frame;
//offset in bytes when parsing a frame (might be useless, needs more testing)
static int frameOffset;
//buffer to write the raw decoded frame.
static int tempBufferSize;
static unsigned char* tempBuffer;
//file descriptor pointing to a location where we will dump raw decoded frames.
static int outfd;
//how many frames at most do we want to dump to this FD ?
static int framesToRec;

/*Load up the h264 codec needed for video decoding.
Perform the initialization steps required by FFmpeg.*/
int video_init_decoder() {

	//initialize libavcodec
	avcodec_register_all();
	//try to load h264
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
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
	//initialize the frame parser (needed to get a whole frame from several packets)
	cpContext = av_parser_init(AV_CODEC_ID_H264);
	//initialize the video packet and frame structures
	av_init_packet(&video_packet);
	current_frame = av_frame_alloc();
	frameOffset = 0;
	
	//open a file to write the decoded frames to
	outfd = open("frames.yuv", O_WRONLY | O_TRUNC | O_CREAT, 0666);
	if(outfd < 0)	perror("Failed to open file");
	framesToRec = 30;
	
	//temp buffer to store raw frame
	tempBufferSize = avpicture_get_size(AV_PIX_FMT_YUV420P, JAKO_VIDEO_WIDTH, JAKO_VIDEO_HEIGHT);
	tempBuffer = calloc(tempBufferSize, 1);
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
	//number of bytes processed by the frame parser and the decoder
	int parsedLen = 0, decodedLen = 0;
	//do we have a whole frame ?
	int complete_frame = 0;
	//how many frames have we decoded ?
	int nb_frames = 0;

	if(buf_size <= 0 || buffer == NULL)
		return 0;

	//parse the video packet. If the parser returns a frame, decode it.
	while(buf_size > 0) {
		//1. parse the newly-received packet. If the parser has assembled a whole frame, store it in the video_packet structure.
		//TODO: confirm/infirm usefulness of frameOffset
		parsedLen = av_parser_parse2(cpContext, context, &video_packet.data, &video_packet.size, buffer, buf_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		
		//2. modify our buffer's data offset to reflect the parser's progression.
		buffer += parsedLen;
		buf_size -= parsedLen;
		frameOffset += parsedLen;
		
		//3. do we have a frame to decode ?
		if(video_packet.size > 0) {
			printf("Packet size : %d\n", video_packet.size);
			decodedLen = avcodec_decode_video2(context, current_frame, &complete_frame, &video_packet);
			if(decodedLen < 0) {
				fprintf(stderr, "Error : couldn't decode frame.\n");
				return -1;
			}
			//If we get there, we should've decoded a frame.
			if(complete_frame) {
				nb_frames++;
				//write the raw frame data in our temporary buffer...
				int picsize = avpicture_layout((const AVPicture*)current_frame, current_frame->format, 
				current_frame->width, current_frame->height, tempBuffer, tempBufferSize);
				//...that we then dump into our file.
				if(framesToRec-- > 0) write(outfd, tempBuffer, picsize);
				printf("Decoded frame : %d bytes, format : %d, size : %dx%d\n", picsize, current_frame->format, current_frame->width, current_frame->height);
				//free the frame's references for reuse
				av_frame_unref(current_frame);
			}
			
			//reinit frame offset for next frame
			frameOffset = 0;
		}

	}

	return nb_frames;
}

void video_stop_decoder() {
	avcodec_close(context);
	//quite recent and not very useful for us, always use avcodec_close for now.
	//avcodec_free_context(&context);
	av_parser_close(cpContext);
	av_frame_free(&current_frame);
	free(tempBuffer);
	close(outfd);
}
