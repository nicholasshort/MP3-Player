#include "mp3_test.h"

#include "main.h"
#include "ff.h"
#include "mp3dec.h"
#include "tad5242.h"
#include "timing.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*
 * File-read ring. Must hold at least the largest possible MP3 frame
 * (~1441 bytes for MPEG1 Layer III @ 320 kbps + a small slop for the
 * sync search). 4 KB gives plenty of headroom.
 */
#define MP3_FILE_BUF_BYTES      4096u

/*
 * Helix can emit up to MAX_NGRAN * MAX_NCHAN * MAX_NSAMP = 2304 int16
 * samples per call (i.e. 1152 stereo frames). The tad5242 half-buffer
 * is 1024 frames, so we stash decoded PCM here and drain it across
 * however many tad5242 half-buffers are needed.
 */
#define MP3_PCM_SCRATCH_SAMPLES (2u * 2u * 576u)

volatile mp3_test_stats_t mp3_test_stats;

static uint8_t  file_buf[MP3_FILE_BUF_BYTES];
static uint32_t file_buf_len;     // valid bytes in file_buf
static uint32_t file_buf_pos;     // read cursor into file_buf
static bool     file_eof;         // f_read returned 0 / short read at EOF

static int16_t  pcm_scratch[MP3_PCM_SCRATCH_SAMPLES];
static uint32_t pcm_scratch_samples;  // valid int16 samples in pcm_scratch
static uint32_t pcm_scratch_pos;      // sample-index cursor into pcm_scratch
static uint32_t pcm_scratch_nchans;   // channel count of the last decoded frame

static void refill_file_buf(FIL* file)
{
    /*
     * Slide leftover tail to the front, then top up from the file.
     */
    uint32_t leftover = file_buf_len - file_buf_pos;
    if (leftover > 0u && file_buf_pos > 0u)
    {
        memmove(file_buf, &file_buf[file_buf_pos], leftover);
    }
    file_buf_len = leftover;
    file_buf_pos = 0u;

    if (file_eof)
    {
        return;
    }

    UINT want = MP3_FILE_BUF_BYTES - file_buf_len;
    UINT got  = 0u;
    FRESULT fr = f_read(file, &file_buf[file_buf_len], want, &got);
    if (fr != FR_OK)
    {
        Error_Handler();
    }

    file_buf_len += (uint32_t)got;
    if (got < want)
    {
        file_eof = true;
    }
}

static bool decode_one_frame(HMP3Decoder dec, FIL* file)
{
    /*
     * Make sure we have enough bytes queued to cover the worst-case
     * frame size. If we don't, refill.
     */
    if ((file_buf_len - file_buf_pos) < 2048u && !file_eof)
    {
        refill_file_buf(file);
    }

    int bytes_left = (int)(file_buf_len - file_buf_pos);
    if (bytes_left <= 0)
    {
        return false;
    }

    int sync_offset = MP3FindSyncWord(&file_buf[file_buf_pos], bytes_left);
    if (sync_offset < 0)
    {
        mp3_test_stats.sync_losses++;
        file_buf_pos = file_buf_len;
        return false;
    }
    file_buf_pos += (uint32_t)sync_offset;
    bytes_left   -= sync_offset;

    unsigned char* in_ptr = &file_buf[file_buf_pos];

    stopwatch_t sw;
    stopwatch_start(&sw);

    int err = MP3Decode(
        dec,
        &in_ptr,
        &bytes_left,
        pcm_scratch,
        0
    );

    uint32_t dt_us = stopwatch_end(&sw);
    mp3_test_stats.last_decode_time_us = dt_us;
    if (dt_us > mp3_test_stats.max_decode_time_us)
    {
        mp3_test_stats.max_decode_time_us = dt_us;
    }

    /* Advance the read cursor by however many bytes Helix consumed. */
    file_buf_pos = (uint32_t)(in_ptr - file_buf);

    if (err != ERR_MP3_NONE)
    {
        mp3_test_stats.decode_errors++;

        if (err == ERR_MP3_INDATA_UNDERFLOW || err == ERR_MP3_MAINDATA_UNDERFLOW)
        {
            refill_file_buf(file);
        }
        else
        {
            if (file_buf_pos < file_buf_len)
            {
                file_buf_pos++;
            }
        }
        return false;
    }

    MP3FrameInfo info;
    MP3GetLastFrameInfo(dec, &info);

    mp3_test_stats.frames_decoded++;
    mp3_test_stats.last_frame_bitrate      = (uint32_t)info.bitrate;
    mp3_test_stats.last_frame_samprate     = (uint32_t)info.samprate;
    mp3_test_stats.last_frame_nchans       = (uint16_t)info.nChans;
    mp3_test_stats.last_frame_output_samps = (uint16_t)info.outputSamps;

    pcm_scratch_samples = (uint32_t)info.outputSamps;
    pcm_scratch_pos     = 0u;
    pcm_scratch_nchans  = (uint32_t)info.nChans;

    return true;
}

static uint32_t pcm_scratch_remaining(void)
{
    return pcm_scratch_samples - pcm_scratch_pos;
}

static void fill_half_buffer(int32_t* buffer, uint32_t frame_count, HMP3Decoder dec, FIL* file)
{
    /*
     * Same logic as the WAV test in main.c: fill an entire DAC half-buffer
     * with real PCM, then commit. We just keep decoding more MP3 frames
     * until we have enough samples to fill it.
     */
    uint32_t samples_needed = frame_count * 2u;  // stereo
    uint32_t samples_written = 0u;

    while (samples_written < samples_needed)
    {
        if (pcm_scratch_remaining() == 0u)
        {
            if (!decode_one_frame(dec, file))
            {
                /*
                 * Couldn't decode (sync loss, decode error, or EOF).
                 * Pad the rest of this half-buffer with silence and bail.
                 */
                for (uint32_t i = samples_written / 2u; i < frame_count; i++)
                {
                    buffer[2u * i + 0u] = 0;
                    buffer[2u * i + 1u] = 0;
                }
                return;
            }
        }

        uint32_t step_samples = pcm_scratch_remaining();
        if (step_samples > (samples_needed - samples_written))
        {
            step_samples = samples_needed - samples_written;
        }

        /* step_samples is in int16 units; one stereo frame = 2 samples. */
        uint32_t step_frames = step_samples / 2u;

        for (uint32_t i = 0u; i < step_frames; i++)
        {
            int16_t l16;
            int16_t r16;

            if (pcm_scratch_nchans == 2u)
            {
                l16 = pcm_scratch[pcm_scratch_pos++];
                r16 = pcm_scratch[pcm_scratch_pos++];
            }
            else
            {
                int16_t s = pcm_scratch[pcm_scratch_pos++];
                l16 = s;
                r16 = s;
            }

            int32_t l32 = (int32_t)(l16 / 10);
            int32_t r32 = (int32_t)(r16 / 10);

            uint32_t out_frame = (samples_written / 2u) + i;
            buffer[2u * out_frame + 0u] = (int32_t)(((uint32_t)l32) << 16);
            buffer[2u * out_frame + 1u] = (int32_t)(((uint32_t)r32) << 16);
        }

        samples_written += step_frames * 2u;
    }
}

void mp3_test_run(const char* filename)
{
    FIL file;
    FRESULT fr = f_open(&file, filename, FA_READ);
    if (fr != FR_OK)
    {
        Error_Handler();
    }

    HMP3Decoder dec = MP3InitDecoder();
    if (dec == NULL)
    {
        Error_Handler();
    }

    memset((void*)&mp3_test_stats, 0, sizeof(mp3_test_stats));
    file_buf_len        = 0u;
    file_buf_pos        = 0u;
    file_eof            = false;
    pcm_scratch_samples = 0u;
    pcm_scratch_pos     = 0u;
    pcm_scratch_nchans  = 2u;

    refill_file_buf(&file);

    if (tad5242_start() != TAD5242_STREAM_OK)
    {
        Error_Handler();
    }

    while (1)
    {
        int32_t* buffer;
        uint32_t frame_count;
        tad5242_stream_status_e st = tad5242_get_audio_buffer(&buffer, &frame_count);

        if (st == TAD5242_STREAM_BUFFER_FULL)
        {
            continue;
        }
        if (st != TAD5242_STREAM_OK)
        {
            Error_Handler();
        }

        fill_half_buffer(buffer, frame_count, dec, &file);

        tad5242_stream_status_e cs = tad5242_commit_audio_buffer();
        if (cs == TAD5242_STREAM_OK)
        {
            mp3_test_stats.commit_ok_count++;
        }
        else if (cs == TAD5242_STREAM_ERR_UNDERRUN)
        {
            mp3_test_stats.underrun_count++;
        }

        if (file_eof && (file_buf_len - file_buf_pos) == 0u
                     && pcm_scratch_pos >= pcm_scratch_samples)
        {
            break;
        }
    }

    (void)tad5242_stop();
    MP3FreeDecoder(dec);
    f_close(&file);

    while (1) { __NOP(); }
}
