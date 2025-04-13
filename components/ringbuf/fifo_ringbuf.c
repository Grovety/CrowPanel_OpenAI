#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "string.h"

#include "fifo_ringbuf.h"

struct fifo_ringbuf_t {
    void *buffer;
    size_t *item_sizes;
    size_t len;
    size_t item_size;
    UBaseType_t write_idx;   // Next position to write
    UBaseType_t read_idx;    // Next position to read
    UBaseType_t count;       // Next position to read
    SemaphoreHandle_t mutex; // Thread safety
    SemaphoreHandle_t
        data_available; // Counting semaphore for available packets
};

// Initialize the ring buffer
fifo_ringbuf_t *fifo_ringbuf_init(size_t len, size_t item_size) {
    fifo_ringbuf_t *rb = (fifo_ringbuf_t *)malloc(sizeof(fifo_ringbuf_t));
    if (!rb)
        return NULL;
    rb->buffer = malloc(item_size * len);
    if (!rb->buffer) {
        free(rb);
        return NULL;
    }
    rb->item_sizes = (size_t *)malloc(sizeof(size_t) * len);
    if (!rb->item_sizes) {
        free(rb->buffer);
        free(rb);
        return NULL;
    }
    rb->len = len;
    rb->item_size = item_size;

    rb->write_idx = 0;
    rb->read_idx = 0;
    rb->count = 0;

    rb->mutex = xSemaphoreCreateMutex();
    if (!rb->mutex) {
        free(rb->buffer);
        free(rb->item_sizes);
        free(rb);
        return NULL;
    }
    rb->data_available = xSemaphoreCreateCounting(rb->len, 0);
    if (!rb->data_available) {
        vSemaphoreDelete(rb->mutex);
        free(rb->buffer);
        free(rb->item_sizes);
        free(rb);
        return NULL;
    }
    return rb;
}

void fifo_ringbuf_release(fifo_ringbuf_t *rb) {
    if (rb) {
        if (rb->buffer)
            free(rb->buffer);
        if (rb->item_sizes)
            free(rb->item_sizes);
        free(rb);
    }
}

// Write data to the buffer (overwrite old data if full)
size_t fifo_ringbuf_write(fifo_ringbuf_t *rb, const void *data, size_t size) {
    if (size > rb->item_size)
        return 0;

    xSemaphoreTake(rb->mutex, portMAX_DELAY);

    void *dst = (int8_t *)rb->buffer + rb->write_idx * rb->item_size;
    memcpy(dst, data, size);
    rb->item_sizes[rb->write_idx] = size;

    rb->write_idx = (rb->write_idx + 1) % rb->len;

    if (rb->count == rb->len) {
        // Buffer is full. To overwrite, advance the read pointer.
        rb->read_idx = (rb->read_idx + 1) % rb->len;
        // The count remains at rb->len.
        // Do not give the semaphore because from the consumer’s point of view,
        // the number of available packets is unchanged.
    } else {
        // Buffer was not full. Increase the count.
        rb->count++;
        // Signal that new data is available.
        xSemaphoreGive(rb->data_available);
    }
    xSemaphoreGive(rb->mutex);
    return size;
}

// Read oldest unread packet (returns true if data was read)
size_t fifo_ringbuf_read(fifo_ringbuf_t *rb, void *data, size_t max_len,
                         size_t timeout) {
    // Block until a packet is available.
    if (xSemaphoreTake(rb->data_available, timeout) != pdTRUE) {
        return 0;
    }

    xSemaphoreTake(rb->mutex, portMAX_DELAY);

    void *item_data = (int8_t *)rb->buffer + rb->read_idx * rb->item_size;
    size_t item_size = rb->item_sizes[rb->read_idx];
    const size_t copy_len = (item_size < max_len) ? item_size : max_len;
    memcpy(data, item_data, copy_len);

    rb->read_idx = (rb->read_idx + 1) % rb->len;
    rb->count--;

    xSemaphoreGive(rb->mutex);
    return copy_len;
}

int fifo_ringbuf_reset(fifo_ringbuf_t *rb) {
    if (rb) {
        xSemaphoreTake(rb->mutex, portMAX_DELAY);
        while (xSemaphoreTake(rb->data_available, 0) == pdTRUE) {
        }
        rb->write_idx = 0;
        rb->read_idx = 0;
        rb->count = 0;
        xSemaphoreGive(rb->mutex);
        return 0;
    }
    return -1;
}
