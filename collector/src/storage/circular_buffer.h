#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stddef.h>

typedef struct {
    double t;
    double T;
    char s[16];
    char i[4];
    double o;
    double c;
    double h;
    double l;
    double v;
} Candle;

typedef struct {
    Candle *candles;
    size_t capacity;
    size_t head;
    size_t count;
    char stream_name[128];
    int is_ready;
    Candle *temp_buffer;
    size_t temp_count;
    size_t temp_capacity;
} CircularBuffer;

void init_storage();

void storage_push(const char *stream_name, Candle candle);

void storage_push_history(const char *stream_name, Candle *candles, size_t count);

CircularBuffer* get_buffer(const char* stream_name);

size_t circular_to_linear(CircularBuffer *buf, Candle *out);

void free_storage();

#endif
