#include "tad5242.h"
#include "i2s.h"
#include <string.h>

#define I2S_HANDLE      (&hi2s1)

#define AUDIO_FRAMES_PER_HALF       2048u // 1 L and 1 R sample per frame
#define AUDIO_SAMPLES_PER_HALF      (AUDIO_FRAMES_PER_HALF * TAD5242_NUM_CHANNELS)
#define AUDIO_HALFWORDS_PER_HALF    (AUDIO_SAMPLES_PER_HALF * 2u)
#define AUDIO_TOTAL_SAMPLES         (AUDIO_SAMPLES_PER_HALF * 2u)
#define AUDIO_TOTAL_HALFWORDS       (AUDIO_TOTAL_SAMPLES * 2u)

typedef union {
    int32_t     i32[AUDIO_TOTAL_SAMPLES];       // Abstracted buffer to be "loaned out" via get_audio_buffer
    uint16_t    u16[AUDIO_TOTAL_HALFWORDS];     // Properly formatted buffer for DMA (I2S register is 16bit)   
} audio_buffer_u;

static audio_buffer_u audio_buffer; // Use Ping Pong audio buffer

static volatile bool dma_to_play_first_half = true;
static volatile bool first_half_empty  = true;
static volatile bool second_half_empty = true; 

static bool first_half_loaned_out = false; // Keep track of which buffer side loaned out by get_audio_buffer

typedef enum {
    STREAM_STATE_OFF = 0,
    STREAM_STATE_ON,
    STREAM_STATE_PAUSED,
} streaming_state_e;

static streaming_state_e streaming_state = STREAM_STATE_OFF;

bool tad5242_init(void) {

    streaming_state = STREAM_STATE_OFF;
    first_half_empty  = true;
    second_half_empty = true; 
    dma_to_play_first_half = true;
    first_half_loaned_out = false;

    memset(audio_buffer.u16, 0, sizeof(audio_buffer.u16));

    return true;

}

tad5242_stream_status_e tad5242_start(void) {

    if (streaming_state == STREAM_STATE_ON)
        return TAD5242_STREAM_OK;

    memset(audio_buffer.u16, 0, sizeof(audio_buffer.u16));

    if (HAL_I2S_Transmit_DMA(I2S_HANDLE, audio_buffer.u16, AUDIO_TOTAL_SAMPLES) != HAL_OK)
        return TAD5242_STREAM_ERR_I2S;

    streaming_state = STREAM_STATE_ON;

    return TAD5242_STREAM_OK;

}

tad5242_stream_status_e tad5242_stop(void) {

    if (streaming_state == STREAM_STATE_OFF)
        return TAD5242_STREAM_OK;

    if (HAL_I2S_DMAStop(I2S_HANDLE) != HAL_OK)
        return TAD5242_STREAM_ERR_I2S;

    streaming_state = STREAM_STATE_OFF;
    
    return TAD5242_STREAM_OK;

}

tad5242_stream_status_e tad5242_pause(void) {

    if (streaming_state == STREAM_STATE_PAUSED)
        return TAD5242_STREAM_OK;

    if (HAL_I2S_DMAPause(I2S_HANDLE) != HAL_OK)
        return TAD5242_STREAM_ERR_I2S;

    streaming_state = STREAM_STATE_PAUSED;
    
    return TAD5242_STREAM_OK;

}

tad5242_stream_status_e tad5242_resume(void) {

    if (streaming_state != STREAM_STATE_PAUSED)
        return TAD5242_STREAM_ERR_INVALID_STATE;
    
    if (HAL_I2S_DMAResume(I2S_HANDLE) != HAL_OK)
        return TAD5242_STREAM_ERR_I2S;

    streaming_state = STREAM_STATE_ON;

    return TAD5242_STREAM_OK;

}

tad5242_stream_status_e tad5242_get_audio_buffer(int32_t** buffer, uint32_t* frame_count) {

    if (streaming_state != STREAM_STATE_ON)
        return TAD5242_STREAM_NOT_RUNNING;

    if (buffer == NULL || frame_count == NULL)
        return TAD5242_STREAM_ERR_INVALID_ARG;

    if ((dma_to_play_first_half && !second_half_empty) ||
        (!dma_to_play_first_half && !first_half_empty))
        return TAD5242_STREAM_BUFFER_FULL;

    if (dma_to_play_first_half) {
        *buffer = &audio_buffer.i32[AUDIO_SAMPLES_PER_HALF];
        first_half_loaned_out = false;
    }
        
    else {
        *buffer = audio_buffer.i32;
        first_half_loaned_out = true;
    }
        
    
    *frame_count = AUDIO_FRAMES_PER_HALF;

    return TAD5242_STREAM_OK;

}

tad5242_stream_status_e tad5242_commit_audio_buffer(void) {

    if (dma_to_play_first_half == first_half_loaned_out)
        return TAD5242_STREAM_ERR_UNDERRUN;

    if (first_half_loaned_out && !first_half_empty)
        return TAD5242_STREAM_BUFFER_FULL;
    
    if (!first_half_loaned_out && !second_half_empty)
        return TAD5242_STREAM_BUFFER_FULL;


    // I2S convention is to send MSB first
    // STM32 is little endian, so in an int32, the LS HalfWord will exist before the MS HalfWord
    // We must transmit the MS Halfword first, so we need to swap these
    uint32_t start_index = first_half_loaned_out ?  0 : (AUDIO_HALFWORDS_PER_HALF);
    uint32_t end_index   = first_half_loaned_out ?  (AUDIO_HALFWORDS_PER_HALF) : (AUDIO_TOTAL_HALFWORDS);

    for (uint32_t i = start_index; i < end_index; i += 2) {
        uint16_t lshw = audio_buffer.u16[i];
        uint16_t mshw = audio_buffer.u16[i+1];
        
        audio_buffer.u16[i]     = mshw;
        audio_buffer.u16[i+1]   = lshw;
    }
    
    if (first_half_loaned_out)
        first_half_empty = false;
    else
        second_half_empty = false;
        
    return TAD5242_STREAM_OK;

}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef* hi2s) {

    if (hi2s->Instance != I2S_HANDLE->Instance)
        return;
    
    dma_to_play_first_half = false;
    first_half_empty = true;

}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef* hi2s) {

    if (hi2s->Instance != I2S_HANDLE->Instance)
        return;

    dma_to_play_first_half = true;
    second_half_empty = true;

}