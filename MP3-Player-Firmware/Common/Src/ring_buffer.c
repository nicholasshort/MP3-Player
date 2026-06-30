#include "ring_buffer.h"
#include <string.h>

void ring_buffer_queue(ring_buffer_t* ring, void* data) {

    if (!ring || !data)
        return;

    if (ring_buffer_full(ring))
        return;

    memcpy(&ring->buffer[ring->head_index * ring->element_size], data, ring->element_size);

    ring->len++;
    ring->head_index++;

    if (ring->head_index >= ring->buffer_size)
        ring->head_index = 0u;

}

void ring_buffer_dequeue(ring_buffer_t* ring, void* data) {

    if (!ring || !data)
        return;

    if (ring_buffer_empty(ring))
        return;
    
    memcpy(data, &ring->buffer[ring->tail_index * ring->element_size], ring->element_size);

    ring->len--;
    ring->tail_index++;

    if (ring->tail_index >= ring->buffer_size)
        ring->tail_index = 0u;

}

bool ring_buffer_empty(const ring_buffer_t* ring) {
    return (ring->len == 0);
}

bool ring_buffer_full(const ring_buffer_t* ring) {
    return ((ring->len > 0) && (ring->head_index == ring->tail_index));
}