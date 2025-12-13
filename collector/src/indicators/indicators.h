#ifndef INDICATORS_H
#define INDICATORS_H

#include "../storage/circular_buffer.h"
#include <cjson/cJSON.h>

cJSON* calculate_indicators(Candle *candles, size_t count, const char *timeframe);

#endif
