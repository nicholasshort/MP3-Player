#include "get_audio.h"
#include "fatfs.h"
#include "mp3dec.h"
#include "ring_buffer.h"
#include <stddef.h>

#define MP3_RAW_BUFFER_LEN  4096        // Must be greater than 1441 bytes to read maximum possible mp3 audio frame (MPEG1 Layer III @ 320 kbps)
#define DECODED_BUFFER_LEN  (1152 * 2)  // One MP3 frame can output a max of 1152 samples per channel

static FIL file;
static HMP3Decoder mp3_decoder;

static audio_format_t audio_format;

static bool initialized = false;
static bool file_opened = false;

static uint8_t mp3_raw_buffer[MP3_RAW_BUFFER_LEN];
static uint32_t mp3_raw_buffer_bytes_total = 0;
static uint32_t mp3_raw_buffer_bytes_available = 0;

static int16_t decoded_buffer[DECODED_BUFFER_LEN];
static uint32_t decoded_buffer_bytes_total = 0;
static uint32_t decoded_buffer_bytes_available = 0;

static get_audio_status_e decode_next_mp3_frame(void) {

    UINT bytes_read;
    FRESULT fr = f_read(&file, mp3_raw_buffer, MP3_RAW_BUFFER_LEN, &bytes_read);

    if (fr != FR_OK)
        return GET_AUDIO_ERR_FILE;

    if (bytes_read == 0u && f_eof(&file))
        return GET_AUDIO_EOF;

    uint32_t bytes_left = MP3_RAW_BUFFER_LEN - bytes_read;
    static uint8_t* mp3_decode_pointer = mp3_raw_buffer;

    int32_t offset = MP3FindSyncWord(mp3_raw_buffer, bytes_read); // Go to next start of next frame
    mp3_decode_pointer += offset;

    MP3FrameInfo frame_info;
    int32_t mp3_err = MP3GetNextFrameInfo(mp3_decoder, &frame_info, mp3_decode_pointer);

    if (mp3_err != ERR_MP3_NONE)
        return GET_AUDIO_ERR_DECODE;

    mp3_err = MP3Decode(mp3_decoder, &mp3_decode_pointer, &bytes_left, decoded_buffer, 0);

    if (mp3_err != ERR_MP3_NONE)
        return GET_AUDIO_ERR_DECODE;

    MP3GetLastFrameInfo(mp3_decoder, &frame_info);
    decoded_buffer_bytes_available += frame_info.outputSamps;

    return GET_AUDIO_OK;

}


get_audio_status_e get_audio_init(audio_format_t format) {

    mp3_decoder = MP3InitDecoder();
    if (mp3_decoder == NULL)
        return GET_AUDIO_ERR_DECODE;
    
    audio_format = format;
    initialized = true;
    
    return GET_AUDIO_OK;

}

get_audio_status_e get_audio_open(const char* path) {

    if (!initialized)
        return GET_AUDIO_ERR_NOT_INITIALIZED;

    FRESULT fr = f_open(&file, path, FA_READ);
    if (fr != FR_OK) {
        file_opened = false;
        return GET_AUDIO_ERR_FILE;
    }
        
    // Skip ID3V2 Header
    uint8_t id3v2_header[10];
    UINT bytes_read;
    fr = f_read(&file, id3v2_header, sizeof(id3v2_header), &bytes_read);
    if (fr != FR_OK) {
        file_opened = false;
        return GET_AUDIO_ERR_FILE;
    }
    
    // Ensure it is an id3v2 header    
    if (id3v2_header[0] == 'I' && id3v2_header[1] == 'D' && id3v2_header[2] == '3') {
        // Get starting mp3 frame offset (Big Endian, 7th bit always 0 to avoid forming synch byte pattern)
        uint32_t starting_frame_offset = (((id3v2_header[6] & 0x7F) << 21) | ((id3v2_header[7] & 0x7F) << 14) | ((id3v2_header[8] & 0x7F) << 7) | (id3v2_header[9] & 0x7F));
        fr = f_lseek(&file, starting_frame_offset);
        if (fr != FR_OK) {
            file_opened = false;
            return GET_AUDIO_ERR_FILE;
        }
    }
    // No id3v2 header present. Assume start of file is frame 1
    else {
        f_rewind(&file);
    }

    file_opened = true;

    return GET_AUDIO_OK;

}

/*
    There are 3 buffers at play here:
        1) static uint8_t mp3_raw_buffer[]: Raw MP3 file data buffer to be consumed by MP3Decode
        2) static int16_t decoded_buffer[]: Decoded buffer cache. Required since void* buffer may be too small to consume all decoded
                                            mp3 data from Decode(mp3_raw_buffer).
        3) void* buffer:                    DAC buffer
    The data goes from mp3_raw_buffer[] -> decoded_buffer[] -> buffer

    This function will always fill void* buffer completely on each call. Will zero pad if necessary
*/
get_audio_status_e get_audio_read(void* buffer, uint32_t audio_frame_count) {

    if (!initialized)
        return GET_AUDIO_ERR_NOT_INITIALIZED;
    
    if (!buffer)
        return GET_AUDIO_ERR_INVALID_ARG;

    uint32_t bytes_to_copy = audio_frame_count * audio_format.channels * audio_format.bits_per_sample / 8u;
    uint32_t bytes_copied = 0u;

    while (bytes_copied < bytes_to_copy) {

        // Transfer from decoded buffer cache first
        uint32_t free_bytes_in_buffer           = (bytes_to_copy - bytes_copied);
        uint32_t bytes_to_transfer              = (free_bytes_in_buffer < decoded_buffer_bytes_available) ? free_bytes_in_buffer : decoded_buffer_bytes_available;
        memcpy((uint8_t*)buffer + (bytes_copied), (uint8_t*)decoded_buffer + (decoded_buffer_bytes_total - decoded_buffer_bytes_available), bytes_to_transfer);
        decoded_buffer_bytes_available -= bytes_to_transfer;
        bytes_copied += bytes_to_transfer;

        // Check if buffer is full
        if (bytes_copied >= bytes_to_copy)
            break;
        
        // Fill decoded_buffer from disk if no data available 
        if (decoded_buffer_bytes_available <= 0)
            decode_next_mp3_frame();
        
    }

    return GET_AUDIO_OK;

}

get_audio_status_e get_audio_close(void) {

    if (!initialized)
        return GET_AUDIO_ERR_NOT_INITIALIZED;

    return GET_AUDIO_OK;

}