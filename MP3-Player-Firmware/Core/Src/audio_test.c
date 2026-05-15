#include "audio_test.h"

#include "i2s.h"
#include <math.h>
#include <stdint.h>

#define AUDIO_SAMPLE_RATE_HZ     48000u
#define SINE_FREQ_HZ             100u
#define SINE_TABLE_SIZE          (AUDIO_SAMPLE_RATE_HZ / SINE_FREQ_HZ)

/*
 * 480 frames = 10 complete periods of a 1 kHz sine at 48 kHz.
 * This avoids a discontinuity when circular DMA wraps.
 */
#define AUDIO_BUFFER_FRAMES      480u
#define AUDIO_CHANNELS           2u
#define HALFWORDS_PER_SAMPLE     2u

/*
 * For 32-bit stereo I2S, each audio frame is:
 *
 * Left  channel: 32 bits = 2 x uint16_t DMA writes
 * Right channel: 32 bits = 2 x uint16_t DMA writes
 *
 * Buffer layout:
 *
 * audio_buffer[0] = left high halfword
 * audio_buffer[1] = left low  halfword
 * audio_buffer[2] = right high halfword
 * audio_buffer[3] = right low  halfword
 * audio_buffer[4] = next left high halfword
 * ...
 */
static uint16_t audio_buffer[AUDIO_BUFFER_FRAMES * AUDIO_CHANNELS * HALFWORDS_PER_SAMPLE];

static int32_t sine_table[SINE_TABLE_SIZE];

static void sine_table_init(void)
{
    /*
     * Start quiet. Increase later once the waveform looks correct.
     */
    const float amplitude = 0.1f * 2147483647.0f;

    for (uint32_t i = 0; i < SINE_TABLE_SIZE; i++)
    {
        float phase = 2.0f * 3.14159265359f * (float)i / (float)SINE_TABLE_SIZE;
        sine_table[i] = (int32_t)(amplitude * sinf(phase));
    }
}

static void audio_buffer_put_frame(uint32_t frame, int32_t left, int32_t right)
{
    uint32_t i = frame * 4u;

    /*
     * Pack high halfword first.
     *
     * Do NOT store as uint32_t and cast to uint16_t*, because little-endian
     * memory order sends the low halfword first, which is wrong for this I2S use.
     */

    audio_buffer[i + 0u] = (uint16_t)(((uint32_t)left  >> 16) & 0xFFFFu);
    audio_buffer[i + 1u] = (uint16_t)(((uint32_t)left  >>  0) & 0xFFFFu);

    audio_buffer[i + 2u] = (uint16_t)(((uint32_t)right >> 16) & 0xFFFFu);
    audio_buffer[i + 3u] = (uint16_t)(((uint32_t)right >>  0) & 0xFFFFu);
}

static void fill_audio_buffer(void)
{
    uint32_t phase = 0;

    for (uint32_t frame = 0; frame < AUDIO_BUFFER_FRAMES; frame++)
    {
        int32_t sample = sine_table[phase];

        phase++;
        if (phase >= SINE_TABLE_SIZE)
        {
            phase = 0;
        }

        audio_buffer_put_frame(frame, sample, sample);
    }
}

void audio_test_start(void)
{
    sine_table_init();
    fill_audio_buffer();

    /*
     * Important STM32 HAL I2S quirk:
     *
     * In 32-bit I2S mode, HAL_I2S_Transmit_DMA() expects the number
     * of 32-bit audio samples, not the number of uint16_t halfwords.
     *
     * HAL internally doubles the DMA transfer count because each
     * 32-bit I2S sample is sent as two 16-bit writes.
     *
     * For stereo:
     * 480 frames × 2 channels = 960 32-bit samples.
     * HAL turns that into 1920 halfword DMA transfers.
     */
    HAL_StatusTypeDef st = HAL_I2S_Transmit_DMA(
        &hi2s1,
        audio_buffer,
        AUDIO_BUFFER_FRAMES * AUDIO_CHANNELS
    );

    if (st != HAL_OK)
    {
        Error_Handler();
    }
}

void audio_test_stop(void)
{
    HAL_I2S_DMAStop(&hi2s1);
}