#ifndef AUDIO_PLAYER
#define AUDIO_PLAYER

extern "C" {
    #include <soundio/soundio.h>
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <atomic>
#include <algorithm>

struct AudioBuffer {
    std::vector<uint8_t> lch_data = {};
    std::vector<uint8_t> rch_data = {};
    size_t idx = 0;
    uint8_t step = 8;
};

// static const float PI = 3.1415926535f;
// static float seconds_offset = 0.0f;
void write_callback(SoundIoOutStream *outstream,
        int frame_count_min, int frame_count_max)
{
    const SoundIoChannelLayout *layout = &outstream->layout;
    SoundIoChannelArea *areas;
    auto audio_buffer = (AudioBuffer*)outstream->userdata;

    int frames_left = std::min(audio_buffer->lch_data.size() - audio_buffer->idx, (size_t) frame_count_max);
    int err;


    const auto sample_size_in_bytes = 4;
    auto rch_data = audio_buffer->rch_data.data();
    auto lch_data = audio_buffer->lch_data.data();
    auto audio_buffer_idx = audio_buffer->idx;
    auto lch_first_byte_in_sample_ptr = lch_data + audio_buffer_idx;
    auto rch_first_byte_in_sample_ptr = rch_data + audio_buffer_idx;


    while (frames_left > 0) {
        int frame_count = frames_left;

        if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
            fprintf(stderr, "%s\n", soundio_strerror(err));
            exit(1);
        }

        if (!frame_count)
            break;


        for (int frame = 0; frame < frame_count; frame += 1) {

            lch_first_byte_in_sample_ptr = lch_data + audio_buffer_idx;
            rch_first_byte_in_sample_ptr = rch_data + audio_buffer_idx;

            // for (int channel = 0; channel < layout->channel_count; channel += 1) {
            //     auto curr_area_ptr = areas[channel].ptr + areas[channel].step * frame;
            //     memcpy(curr_area_ptr, lch_first_byte_in_sample_ptr, outstream->bytes_per_sample);
            // }

            auto curr_area_ptr = areas[0].ptr + areas[0].step * frame;
            memcpy(curr_area_ptr, lch_first_byte_in_sample_ptr, outstream->bytes_per_sample);

            curr_area_ptr = areas[1].ptr + areas[1].step * frame;
            memcpy(curr_area_ptr, rch_first_byte_in_sample_ptr, outstream->bytes_per_sample);

            audio_buffer_idx += sample_size_in_bytes;
        }

        if ((err = soundio_outstream_end_write(outstream))) {
            fprintf(stderr, "%s\n", soundio_strerror(err));
            exit(1);
        }

        frames_left -= frame_count;
        audio_buffer->idx = audio_buffer_idx;
    }
}

auto underflow_callback (SoundIoOutStream *outstream) {
    printf("Underflow\n");
}


auto init_audio_player (AudioBuffer &audio_buffer) {
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
    outstream->userdata = &audio_buffer;
    outstream->write_callback = write_callback;
    outstream->underflow_callback = underflow_callback;

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

    for (;;) {
        soundio_wait_events(soundio);
    }

    soundio_outstream_destroy(outstream);
    soundio_device_unref(device);
    soundio_destroy(soundio);
}

#endif
