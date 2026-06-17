#ifndef TAD5242_H
#define TAD5242_H

#include <stdint.h>
#include <stdbool.h>

#define TAD5242_SAMPLE_RATE_HZ      48000u  // 48kHz sample rate
#define TAD5242_SAMPLE_SIZE_BYTES   2u      // 16 bit samples
#define TAD5242_NUM_CHANNELS        2u      // Stereo Audio

typedef enum {
    TAD5242_STREAM_OK = 0,
    TAD5242_STREAM_NOT_RUNNING,
    TAD5242_STREAM_BUFFER_FULL,

    TAD5242_STREAM_ERR_INVALID_ARG,
    TAD5242_STREAM_ERR_INVALID_STATE,
    TAD5242_STREAM_ERR_I2S,
    TAD5242_STREAM_ERR_UNDERRUN
} tad5242_stream_status_e;

bool tad5242_init(void);
tad5242_stream_status_e tad5242_start(void);
tad5242_stream_status_e tad5242_stop(void);
tad5242_stream_status_e tad5242_pause(void);
tad5242_stream_status_e tad5242_resume(void);
tad5242_stream_status_e tad5242_get_audio_buffer(int32_t** buffer, uint32_t* frame_count);
tad5242_stream_status_e tad5242_commit_audio_buffer(void);

#endif // TAD5242_H