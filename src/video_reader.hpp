#ifndef VIDEO_READER
#define VIDEO_READER

#define SDL_MAIN_HANDLED
extern "C" {
#include <SDL2/SDL.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <memory>
#include <chrono>
#include "frame_buffer.hpp"

struct VideoReader {
	AVFormatContext* av_format_ctx = avformat_alloc_context();

	AVRational time_base;

	std::chrono::steady_clock::time_point first_frame_begin_time;
	uint64_t frame_pst_microsec;
	int8_t video_stream_index = -1;
	AVCodecParameters* av_codec_params = nullptr;
	AVCodec* av_codec = nullptr;
	AVCodecContext* av_codec_ctx = nullptr;

	AVFrame* av_frame = av_frame_alloc();
	AVPacket* av_packet = av_packet_alloc();

	SwsContext* sws_scaler_ctx = nullptr;

	uint16_t width, height;

	VideoReader(const char* filename) {

		if (!av_frame || !av_packet) {
			fprintf(stderr, "Couldn't allocate a packet or frame\n");
			return;
		}

		if (!av_format_ctx) {
			fprintf(stderr, "Couldn't create AVFormatContext\n");
			return;
		}

		if (avformat_open_input(&av_format_ctx, filename, nullptr, nullptr) < 0) {
			fprintf(stderr, "Couldn't open video file: %s\n", filename);
			return;
		}

		for (auto i = 0; i < av_format_ctx->nb_streams; ++i) {
			auto stream = av_format_ctx->streams[i];

			av_codec_params = av_format_ctx->streams[i]->codecpar;
			av_codec = avcodec_find_decoder(av_codec_params->codec_id);

			if (!av_codec) {
				continue;
			}

			if (av_codec_params->codec_type == AVMEDIA_TYPE_VIDEO) {
				video_stream_index = i;
				time_base = stream->time_base; 
				break;
			}
		}

		if (video_stream_index == -1) {
			fprintf(stderr, "Couldn't find a video stream\n");
			return;
		}

		av_codec_ctx = avcodec_alloc_context3(av_codec);

		if (!av_codec_ctx) {
			fprintf(stderr, "Couldn't allocate AVCodecContext\n");
			return;
		}

		if (avcodec_parameters_to_context(av_codec_ctx, av_codec_params) < 0) {
			fprintf(stderr, "Couldn't initialize AVCodecContext\n");
			return;
		}

		if (avcodec_open2(av_codec_ctx, av_codec, nullptr) < 0) {
			fprintf(stderr, "Couldn't open codec\n");
			return;
		}

		width = av_codec_params->width;
		height = av_codec_params->height;
	}

	~VideoReader() {
		if (av_format_ctx) {
			avformat_close_input(&av_format_ctx);
			avformat_free_context(av_format_ctx);
		}
		if (av_codec_ctx) avcodec_free_context(&av_codec_ctx);
		if (av_packet) av_packet_free(&av_packet);
		if (av_frame) av_frame_free(&av_frame);
		if (sws_scaler_ctx) sws_freeContext(sws_scaler_ctx);
	}

	auto read_first_frame() {
	}

	auto read_single_frame(FrameBuffer &frame_buffer) {
		while (av_read_frame(av_format_ctx, av_packet) >= 0) {
			if (av_packet->stream_index != video_stream_index) {
				av_packet_unref(av_packet);
				continue;
			}

			auto response = avcodec_send_packet(av_codec_ctx, av_packet);

			if (response < 0) {
				fprintf(stderr, "Failed to decode packet\n");
				av_packet_unref(av_packet);
				return;
			}

			response = avcodec_receive_frame(av_codec_ctx, av_frame);

			if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
				av_packet_unref(av_packet);
				continue;
			}
			else if (response < 0) {
				fprintf(stderr, "Failed to receive frame\n");
				av_packet_unref(av_packet);
				return;
			}

			av_packet_unref(av_packet);
			break;
		}

		frame_pst_microsec = (double) av_frame->pts * 1'000'000 * (double) time_base.num / (double) time_base.den; 

		if (!sws_scaler_ctx) {
			sws_scaler_ctx = sws_getContext(width, height, av_codec_ctx->pix_fmt,
				width, height, AV_PIX_FMT_RGB0,
				SWS_BILINEAR, nullptr, nullptr, nullptr);

			if (!sws_scaler_ctx) {
				fprintf(stderr, "Couldn't initialize sw scaler\n");
				return;
			}

			first_frame_begin_time = std::chrono::steady_clock::now();
		}

		auto copy_scaled_data = [this](uint8_t* frame_data_rawptr){
			uint8_t* dest[4] = { frame_data_rawptr, nullptr, nullptr, nullptr };
			const int dest_linesize[4] = { width * 4, 0, 0, 0 };
			sws_scale(sws_scaler_ctx, av_frame->data, av_frame->linesize, 0, height, dest, dest_linesize);
		};

		frame_buffer.write_frame(copy_scaled_data);
	}
};


#endif