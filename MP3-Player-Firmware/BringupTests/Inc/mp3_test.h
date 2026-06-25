#ifndef MP3_TEST_H
#define MP3_TEST_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Very simple MP3 decode + DAC playback throughput test.
 *
 * Streams a hardcoded MP3 file from the SD card, decodes one frame at a
 * time with Helix, and feeds the resulting PCM into the tad5242 ping-pong
 * I2S buffer. The only thing this test is trying to answer is:
 *
 *   "Is the MCU fast enough to keep the DAC fed without underruns?"
 *
 * Watch mp3_test_stats in the debugger while it runs.
 *
 *   - max_decode_time_us : worst-case MP3Decode() duration.
 *                          Budget at 48 kHz / 1152 samples per frame is
 *                          24000 us. Anything well under that is fine.
 *
 *   - underrun_count     : the ground truth. Should stay at 0 after the
 *                          first frame or two of priming.
 */

typedef struct {
    uint32_t frames_decoded;           // successful MP3 frames decoded
    uint32_t decode_errors;            // non-fatal Helix decode errors
    uint32_t sync_losses;              // times we had to re-find sync
    uint32_t underrun_count;           // commits that returned UNDERRUN
    uint32_t commit_ok_count;          // commits that succeeded
    uint32_t last_decode_time_us;      // last MP3Decode() duration
    uint32_t max_decode_time_us;       // worst-case MP3Decode() duration
    uint32_t last_frame_bitrate;       // last frame: bitrate (bps)
    uint32_t last_frame_samprate;      // last frame: sample rate (Hz)
    uint16_t last_frame_nchans;        // last frame: channels
    uint16_t last_frame_output_samps;  // last frame: PCM samples produced
} mp3_test_stats_t;

extern volatile mp3_test_stats_t mp3_test_stats;

/* Blocks forever. Call after f_mount + tad5242_init + timing_init. */
void mp3_test_run(const char* filename);

#endif // MP3_TEST_H
