#include "circular_buffer.h"
#include "../config/config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static CircularBuffer *buffers = NULL;
static size_t buffer_count = 0;

void init_storage() {
    cJSON *symbols = cJSON_GetObjectItem(app_config.json, "symbols");
    cJSON *timeframes = cJSON_GetObjectItem(app_config.json, "timeframes");
    
    size_t s_count = cJSON_GetArraySize(symbols);
    size_t t_count = cJSON_GetArraySize(timeframes);
    
    buffer_count = s_count * t_count;
    buffers = malloc(sizeof(CircularBuffer) * buffer_count);
    
    size_t idx = 0;
    cJSON *sym = NULL;
    cJSON_ArrayForEach(sym, symbols) {
        cJSON *tf = NULL;
        cJSON_ArrayForEach(tf, timeframes) {
            char *s = sym->valuestring;
            char clean_sym[64];
            int j = 0;
            for(int i=0; s[i]; i++) {
                if(s[i] != '/') clean_sym[j++] = tolower(s[i]);
            }
            clean_sym[j] = '\0';
            
            snprintf(buffers[idx].stream_name, 128, "%s@kline_%s", clean_sym, tf->valuestring);
            
            buffers[idx].capacity = app_config.buffer_size;
            buffers[idx].candles = malloc(sizeof(Candle) * buffers[idx].capacity);
            buffers[idx].head = 0;
            buffers[idx].count = 0;
            buffers[idx].is_ready = 0;
            buffers[idx].temp_capacity = 100;
            buffers[idx].temp_buffer = malloc(sizeof(Candle) * buffers[idx].temp_capacity);
            buffers[idx].temp_count = 0;
            
            printf("Initialized buffer for %s with size %d\n", buffers[idx].stream_name, (int)buffers[idx].capacity);
            idx++;
        }
    }
}

void storage_push(const char *stream_name, Candle candle) {
    CircularBuffer *cb = get_buffer(stream_name);
    if (!cb) {
        printf("Warning: No buffer found for stream %s\n", stream_name);
        return;
    }

    if (!cb->is_ready) {
        if (cb->temp_count < cb->temp_capacity) {
            cb->temp_buffer[cb->temp_count++] = candle;
            printf("Stored temp candle for %s. Count: %zu\n", stream_name, cb->temp_count);
        } else {
            printf("Warning: Temp buffer full for %s, dropping candle\n", stream_name);
        }
        return;
    }

    cb->candles[cb->head] = candle;
    cb->head = (cb->head + 1) % cb->capacity;
    if (cb->count < cb->capacity) {
        cb->count++;
    }
}

void storage_push_history(const char *stream_name, Candle *candles, size_t count) {
    CircularBuffer *cb = get_buffer(stream_name);
    if (!cb) return;

    for (size_t i = 0; i < count; i++) {
        cb->candles[cb->head] = candles[i];
        cb->head = (cb->head + 1) % cb->capacity;
        if (cb->count < cb->capacity) {
            cb->count++;
        }
    }

    double last_hist_time = 0;
    if (count > 0) last_hist_time = candles[count-1].t;

    for (size_t i = 0; i < cb->temp_count; i++) {
        if (cb->temp_buffer[i].t > last_hist_time) {
            cb->candles[cb->head] = cb->temp_buffer[i];
            cb->head = (cb->head + 1) % cb->capacity;
            if (cb->count < cb->capacity) {
                cb->count++;
            }
        }
    }

    // 3. Mark as ready and free temp buffer
    cb->is_ready = 1;
    free(cb->temp_buffer);
    cb->temp_buffer = NULL;
    cb->temp_count = 0;
    
    printf("History loaded for %s. Total count: %zu\n", stream_name, cb->count);
}

CircularBuffer* get_buffer(const char* stream_name) {
    if (!buffers) return NULL;
    for (size_t i = 0; i < buffer_count; i++) {
        if (strcmp(buffers[i].stream_name, stream_name) == 0) {
            return &buffers[i];
        }
    }
    return NULL;
}

size_t circular_to_linear(CircularBuffer *buf, Candle *out) {
    if (!buf || buf->count == 0) return 0;

    size_t start = (buf->head - buf->count + buf->capacity) % buf->capacity;

    for (size_t i = 0; i < buf->count; i++) {
        size_t idx = (start + i) % buf->capacity;
        out[i] = buf->candles[idx];
    }

    return buf->count;
}

void free_storage() {
    if (buffers) {
        for (size_t i = 0; i < buffer_count; i++) {
            free(buffers[i].candles);
            if (buffers[i].temp_buffer) free(buffers[i].temp_buffer);
        }
        free(buffers);
        buffers = NULL;
    }
}
