#include <stdio.h>
#include <memory>
#include <chrono>
#include <thread>
#include <iostream>
#include <atomic>

#include "video_reader.hpp"
#include "audio_player.hpp"

#define SDL_MAIN_HANDLED
extern "C" {
#include <SDL2/SDL.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <soundio/soundio.h>
}

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540

int main() {

	VideoReader video_reader_state("C:\\Users\\dudko\\OneDrive\\Desktop\\rotating_globe.mp4");

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

	uint8_t* video_frame_data = nullptr;

	SDL_Rect stretchRect;
	stretchRect.x = 0;
	stretchRect.y = 0;
	stretchRect.w = SCREEN_WIDTH;
	stretchRect.h = SCREEN_HEIGHT;

	auto quit = false;
	SDL_Event e;

	std::chrono::steady_clock::time_point frame_begin_time, frame_end_time;
	std::chrono::microseconds duration_since_start;

	int err;
    struct SoundIo *soundio = soundio_create();
    if (!soundio) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    if ((err = soundio_connect(soundio))) {
        fprintf(stderr, "error connecting: %s", soundio_strerror(err));
        return 1;
    }

    soundio_flush_events(soundio);

    int default_out_device_index = soundio_default_output_device_index(soundio);
    if (default_out_device_index < 0) {
        fprintf(stderr, "no output device found");
        return 1;
    }

    struct SoundIoDevice *device = soundio_get_output_device(soundio, default_out_device_index);
    if (!device) {
        fprintf(stderr, "out of memory");
        return 1;
    }

    fprintf(stderr, "Output device: %s\n", device->name);

    struct SoundIoOutStream *outstream = soundio_outstream_create(device);
    outstream->format = SoundIoFormatFloat32NE;
    outstream->write_callback = write_callback;

    if ((err = soundio_outstream_open(outstream))) {
        fprintf(stderr, "unable to open device: %s", soundio_strerror(err));
        return 1;
    }

    if (outstream->layout_error)
        fprintf(stderr, "unable to set channel layout: %s\n", soundio_strerror(outstream->layout_error));

    if ((err = soundio_outstream_start(outstream))) {
        fprintf(stderr, "unable to start device: %s", soundio_strerror(err));
        return 1;
    }


	while (!quit) {

	    // soundio_wait_events(soundio);

		// frame_begin_time = std::chrono::steady_clock::now();

		video_frame_data = video_reader_state.read_single_frame();

		if (!video_frame_data) {
			continue;
		}

		auto sdl_frame_surface = SDL_CreateRGBSurfaceFrom(
			(void*)video_frame_data,
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
			return 1;
		}

		if (SDL_BlitScaled(sdl_frame_surface, nullptr, SDL_GetWindowSurface(sdl_window), &stretchRect) < 0) {
			fprintf(stderr, "Couldn't blit the frame surface: %s\n", SDL_GetError());
			return 1;
		}

		SDL_UpdateWindowSurface(sdl_window);

		SDL_FreeSurface(sdl_frame_surface);

		frame_end_time = std::chrono::steady_clock::now();

		duration_since_start = std::chrono::duration_cast<std::chrono::microseconds>(frame_end_time - video_reader_state.first_frame_begin_time);

		auto sleep_time = std::chrono::microseconds(video_reader_state.frame_pst_microsec) - duration_since_start;
		std::this_thread::sleep_for(sleep_time);

		if (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
		}
	}

	SDL_DestroyWindow(sdl_window);
	SDL_Quit();

	soundio_outstream_destroy(outstream);
    soundio_device_unref(device);
    soundio_destroy(soundio);
}