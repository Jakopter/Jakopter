/* Jakopter
 * Copyright © 2014 - 2015 Hector Labanca, Thibaud Hulin, Thibaut Rousseau
 * Copyright © 2015 ALF@INRIA
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */
#include "video_decode.h"


static AVCodec* codec;
static AVCodecContext* context;
static AVCodecParserContext* cpContext;
static AVPacket video_packet;
static AVFrame* current_frame;
//offset in bytes when parsing a frame (might be useless, needs more testing)
static int frameOffset;
//buffer to write the raw decoded frame.
static int tempBufferSize = 0;
static unsigned char* tempBuffer = NULL;
//current video size. Used to check whether the size has changed, and tempBuffer reallocation is needed.
static int current_width = 0, current_height = 0;

/*Load up the h264 codec needed for video decoding.
Perform the initialization steps required by FFmpeg.*/
int video_init_decoder() {
	//initialize libavcodec
	avcodec_register_all();

	//try to load h264
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (codec == NULL) {
		fprintf(stderr, "[~][decode][FFmpeg] Counldn't find needed codec H264 for video decoding.\n");
		return -1;
	}

	//inilialize the ffmpeg codec context
	context = avcodec_alloc_context3(codec);
	if (avcodec_open2(context, codec, NULL) < 0) {
		fprintf(stderr, "[~][decode][FFmpeg] Couldn't open codec.\n");
		return -1;
	}

	//initialize the frame parser (needed to get a whole frame from several packets)
	cpContext = av_parser_init(AV_CODEC_ID_H264);
	//initialize the video packet and frame structures
	av_init_packet(&video_packet);
	current_frame = av_frame_alloc();
	frameOffset = 0;

	//prevent h264 from logging error messages that we have no interest in
	av_log_set_level(JAKO_FFMPEG_LOG);

	//temp buffer to store raw frame; for now, don't make assumptions on its size,
	//wait til frames come in to allocate it.
	tempBufferSize = 0;
	tempBuffer = NULL;
	current_width = 0;
	current_height = 0;
	return 0;
}

/*
* Allocate tempBuffer so that it can hold a decoded YUV420p frame of the
* size currently specified in the current_frame structure.
* Update current_width/height and tempBufferSize to match these new dimensions.
*/
static int video_alloc_frame_buffer() {
	if (tempBuffer != NULL)
		free(tempBuffer);
	// align linesize should be to 1, otherwise use av_image_get_linesize
	tempBufferSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, current_frame->width, current_frame->height, 1);
	if (tempBufferSize == -1)
		return -1;
	tempBuffer = calloc(tempBufferSize, 1);
	if (tempBuffer == NULL)
		return -1;
	current_width = current_frame->width;
	current_height = current_frame->height;
	return 0;
}

/*
Decode a video buffer.
Returns:
	0 : buffer decoded, but no image produced (incomplete).
	> 0 : decoded n images.
	-1 : error while decoding.
*/
int video_decode_packet(uint8_t* buffer, int buf_size, jakopter_video_frame_t* result) {
	//number of bytes processed by the frame parser and the decoder
	int parsedLen = 0; // decodedLen = 0;
	//do we have a whole frame ?
	int complete_frame = 0;
	//how many frames have we decoded ?
	int nb_frames = 0;

	if (buf_size <= 0 || buffer == NULL)
		return 0;

	//parse the video packet. If the parser returns a frame, decode it.
	while (buf_size > 0) {
		//1. parse the newly-received packet. If the parser has assembled a whole frame, store it in the video_packet structure.
		//TODO: confirm/infirm usefulness of frameOffset
		parsedLen = av_parser_parse2(cpContext, context, &video_packet.data, &video_packet.size, buffer, buf_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

		//2. modify our buffer's data offset to reflect the parser's progression.
		buffer += parsedLen;
		buf_size -= parsedLen;
		frameOffset += parsedLen;

		//3. do we have a frame to decode ?
		// check if the size of video packet parsed is larger than read bytes in order
		// to avoid a reader to read over the end
		//printf("Packet size : %d\n", video_packet.size);
		if (video_packet.size >= AV_INPUT_BUFFER_PADDING_SIZE + buf_size) {
			// video_packet.data >= AV_INPUT_BUFFER_PADDING_SIZE + buf_size
			// decodedLen = avcodec_decode_video2(context, current_frame, &complete_frame, &video_packet);
			// if (decodedLen < 0) {
			// 	fprintf(stderr, "[~][decode] couldn't decode frame.\n");
			// 	return 0;
			// }
			// send packet to the decoder
			if (avcodec_send_packet (context, (const AVPacket *)&video_packet) < 0) {
				fprintf(stderr, "[~][decode] couldn't decode frame.\n");
				return 0;
			}
			//receive frame from the decoder
			complete_frame = avcodec_receive_frame(context, current_frame);
			//If we get there, we should've decoded a frame.
			if (complete_frame == 0) {
				nb_frames++;
				//check if the video size has changed
				if (current_frame->width != current_width || current_frame->height != current_height)
					if (video_alloc_frame_buffer() < 0) {
						fprintf(stderr, "[~][decode] couldn't allocate memory for decoding.\n");
						return -1;
					}
				//write the raw frame data in our temporary buffer
				int picsize = av_image_copy_to_buffer(tempBuffer, tempBufferSize,
                                   					  (const uint8_t**) current_frame->data, 
                                   					  current_frame->linesize,
                                   					  current_frame->format, current_frame->width, 
                                   					  current_frame->height, 1);
				//write the final result in the output structure
				result->pixels = tempBuffer;
				result->w = current_frame->width;
				result->h = current_frame->height;
				result->size = picsize;

				//printf("Decoded frame : %d bytes, format : %d, size : %dx%d\n", picsize, current_frame->format, current_frame->width, current_frame->height);
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
	if (tempBuffer != NULL)
		free(tempBuffer);
}
