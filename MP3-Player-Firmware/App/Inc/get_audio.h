#ifndef GET_AUDIO_H
#define GET_AUDIO_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t sample_rate_hz;
    uint8_t  channels;
    uint8_t  bits_per_sample;
    bool     left_justified;
} audio_format_t;

typedef enum {
    GET_AUDIO_OK,
    GET_AUDIO_EOF,
    GET_AUDIO_ERR_FILE,
    GET_AUDIO_ERR_DECODE,
    GET_AUDIO_ERR_NOT_INITIALIZED,
    GET_AUDIO_ERR_INVALID_ARG,
} get_audio_status_e;

get_audio_status_e get_audio_init(audio_format_t format);
get_audio_status_e get_audio_open(const char* path);
get_audio_status_e get_audio_read(void* buffer, uint32_t audio_frame_count); // Completely fills audio buffer no matter what
get_audio_status_e get_audio_close(void);

#endif  // GET_AUDIO_H