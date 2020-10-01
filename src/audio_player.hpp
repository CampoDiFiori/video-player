extern "C" {
    #include <soundio/soundio.h>
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static const float PI = 3.1415926535f;
static float seconds_offset = 0.0f;
void write_callback(SoundIoOutStream *outstream,
        int frame_count_min, int frame_count_max)
{
    const SoundIoChannelLayout *layout = &outstream->layout;
    float float_sample_rate = outstream->sample_rate;
    float seconds_per_frame = 1.0f / float_sample_rate;
    SoundIoChannelArea *areas;
    int frames_left = frame_count_max;
    int err;

    while (frames_left > 0) {
        int frame_count = frames_left;

        if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
            fprintf(stderr, "%s\n", soundio_strerror(err));
            exit(1);
        }

        if (!frame_count)
            break;

        
        float pitch = 440.0f;
        float radians_per_second = pitch * 2.0f * PI;
        for (int frame = 0; frame < frame_count; frame += 1) {
            float sample = sinf((seconds_offset + frame * seconds_per_frame) * radians_per_second);
            for (int channel = 0; channel < layout->channel_count; channel += 1) {
                float *ptr = (float*)(areas[channel].ptr + areas[channel].step * frame);
                *ptr = sample;
            }
        }
        seconds_offset += seconds_per_frame * frame_count;

        if ((err = soundio_outstream_end_write(outstream))) {
            fprintf(stderr, "%s\n", soundio_strerror(err));
            exit(1);
        }

        frames_left -= frame_count;
    }
}
