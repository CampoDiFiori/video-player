#include <stdio.h>
#include <memory>
#include <chrono>
#include <thread>
#include <iostream>
#include <atomic>

#include "video_reader.hpp"
#include "frame_buffer.hpp"

#define SDL_MAIN_HANDLED
extern "C" {
#include <SDL2/SDL.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540

void read_frames_to_buffer(VideoReader *video_reader, FrameBuffer *frame_buffer, std::atomic<bool> *quit) {
	while (!*quit) {
		video_reader->read_single_frame(*frame_buffer);
	}
}

int main() {
	VideoReader video_reader_state("C:\\Users\\dudko\\OneDrive\\Desktop\\rotating_globe.mp4");
	FrameBuffer frame_buffer(video_reader_state.width, video_reader_state.height);

	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		printf("SDL could not initialize: %s\n", SDL_GetError());
		return 1;
	}

	const auto sdl_window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

	if (!sdl_window) {
		fprintf(stderr, "Couldn't initialize the SDL window: %s\n", SDL_GetError());
		return 1;
	}

	const auto rgb_channel_count = 4;

	const auto sdl_window_format = SDL_GetWindowSurface(sdl_window)->format;

	SDL_Rect stretchRect;
	stretchRect.x = 0;
	stretchRect.y = 0;
	stretchRect.w = SCREEN_WIDTH;
	stretchRect.h = SCREEN_HEIGHT;

	std::atomic<bool> quit (false);
	SDL_Event e;

	std::chrono::steady_clock::time_point frame_begin_time, frame_end_time;
	std::chrono::microseconds duration_since_start;

	std::thread video_reader_thread (read_frames_to_buffer, &video_reader_state, &frame_buffer, &quit);

	auto render_from_buffer = [&](uint8_t* frame_data) {
		auto sdl_frame_surface = SDL_CreateRGBSurfaceFrom(
			(void*) frame_data,
			video_reader_state.width,
			video_reader_state.height,
			rgb_channel_count * 8,          // bits per pixel = 24
			video_reader_state.width * rgb_channel_count,  	// pitch
			0x0000FF,              			// red mask
			0x00FF00,              			// green mask
			0xFF0000,              			// blue mask
			0
		);

		if (!sdl_frame_surface) {
			fprintf(stderr, "Couldn't push frame pixels: %s\n", SDL_GetError());
			return;
		}

		if (SDL_BlitScaled(sdl_frame_surface, nullptr, SDL_GetWindowSurface(sdl_window), &stretchRect) < 0) {
			fprintf(stderr, "Couldn't blit the frame surface: %s\n", SDL_GetError());
			return;
		}

		SDL_UpdateWindowSurface(sdl_window);
		SDL_FreeSurface(sdl_frame_surface);
	};
	
	while (!quit) {

		// frame_begin_time = std::chrono::steady_clock::now();
		

		frame_buffer.read_frame(render_from_buffer);

		frame_end_time = std::chrono::steady_clock::now();

		duration_since_start = std::chrono::duration_cast<std::chrono::microseconds>(frame_end_time - video_reader_state.first_frame_begin_time);

		auto sleep_time = std::chrono::microseconds(video_reader_state.frame_pst_microsec) - duration_since_start;
		std::this_thread::sleep_for(sleep_time);

		printf("Sleeptime:  %d\n", sleep_time);

		if (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
		}
	}

	video_reader_thread.join();

	SDL_DestroyWindow(sdl_window);
	SDL_Quit();
}