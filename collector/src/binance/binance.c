#include "binance.h"
#include "../config/config.h"
#include "../server/server.h"
#include "../storage/circular_buffer.h"
#include "../indicators/indicators.h"
#include <cjson/cJSON.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <curl/curl.h>
#include <immintrin.h>

static inline int64_t fast_atoi64_chunk(const char* ptr, const char* end) {
    int64_t val = 0;
    while (ptr < end) {
        val = val * 10 + (*ptr++ - '0');
    }
    return val;
}

static inline double fast_atof_chunk(const char* ptr, const char* end) {
    double res = 0.0;
    while (ptr < end && *ptr != '.') {
        res = res * 10.0 + (*ptr++ - '0');
    }
    if (ptr < end && *ptr == '.') {
        ptr++;
        double factor = 0.1;
        while (ptr < end) {
            res += (*ptr++ - '0') * factor;
            factor *= 0.1;
        }
    }
    return res;
}

static inline const char* advance_to_value(const char* ptr) {
    while (*ptr != ':') ptr++;
    ptr++; 
    while (*ptr == ' ' || *ptr == '"') ptr++;
    return ptr;
}

static inline const char* find_value_end(const char* ptr) {
    while (*ptr != ',' && *ptr != '}' && *ptr != '"') ptr++;
    return ptr;
}

static inline const char* find_key_3char_avx2(const char* str, char key_char) {
    __m256i quote = _mm256_set1_epi8('"');
    
    while (1) {
        __m256i block = _mm256_loadu_si256((const __m256i*)str);
        __m256i eq = _mm256_cmpeq_epi8(block, quote);
        int mask = _mm256_movemask_epi8(eq);
        
        while (mask) {
            int idx = __builtin_ctz(mask);
            if (str[idx+1] == key_char && str[idx+2] == '"') {
                return str + idx;
            }
            mask &= ~(1 << idx);
        }
        
        __m256i zero = _mm256_setzero_si256();
        if (_mm256_movemask_epi8(_mm256_cmpeq_epi8(block, zero))) return NULL;
        
        str += 32;
    }
}

void parse_candle_avx2(const char* json, Candle* c) {
    const char* ptr = json;
    const char* end;

    ptr = find_key_3char_avx2(ptr, 't');
    if (!ptr) return;
    ptr = advance_to_value(ptr);
    end = find_value_end(ptr);
    c->t = (double)fast_atoi64_chunk(ptr, end);
    ptr = end;

    ptr = find_key_3char_avx2(ptr, 'T');
    if (!ptr) return;
    ptr = advance_to_value(ptr);
    end = find_value_end(ptr);
    c->T = (double)fast_atoi64_chunk(ptr, end);
    ptr = end;

    ptr = find_key_3char_avx2(ptr, 's');
    if (!ptr) return;
    ptr = advance_to_value(ptr);
    end = find_value_end(ptr);
    int len = end - ptr;
    if (len > 15) len = 15;
    memcpy(c->s, ptr, len);
    c->s[len] = 0;
    ptr = end;

    ptr = find_key_3char_avx2(ptr, 'i');
    if (!ptr) return;
    ptr = advance_to_value(ptr);
    end = find_value_end(ptr);
    len = end - ptr;
    if (len > 3) len = 3;
    memcpy(c->i, ptr, len);
    c->i[len] = 0;
    ptr = end;

    ptr = find_key_3char_avx2(ptr, 'o');
    if (!ptr) return;
    ptr = advance_to_value(ptr);
    end = find_value_end(ptr);
    c->o = fast_atof_chunk(ptr, end);
    ptr = end;

    ptr = find_key_3char_avx2(ptr, 'c');
    if (!ptr) return;
    ptr = advance_to_value(ptr);
    end = find_value_end(ptr);
    c->c = fast_atof_chunk(ptr, end);
    ptr = end;

    ptr = find_key_3char_avx2(ptr, 'h');
    if (!ptr) return;
    ptr = advance_to_value(ptr);
    end = find_value_end(ptr);
    c->h = fast_atof_chunk(ptr, end);
    ptr = end;

    ptr = find_key_3char_avx2(ptr, 'l');
    if (!ptr) return;
    ptr = advance_to_value(ptr);
    end = find_value_end(ptr);
    c->l = fast_atof_chunk(ptr, end);
    ptr = end;

    ptr = find_key_3char_avx2(ptr, 'v');
    if (!ptr) return;
    ptr = advance_to_value(ptr);
    end = find_value_end(ptr);
    c->v = fast_atof_chunk(ptr, end);
}

int is_candle_closed_avx2(const char* json) {
    const char* ptr = find_key_3char_avx2(json, 'x');
    if (!ptr) return 0;
    ptr = advance_to_value(ptr);
    return *ptr == 't';
}

// struct lws *wsi_binance = NULL;

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(!ptr) {
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

void fetch_historical_data(struct lws_context *context) {
    (void)context; 
    cJSON *symbols = cJSON_GetObjectItem(app_config.json, "symbols");
    cJSON *timeframes = cJSON_GetObjectItem(app_config.json, "timeframes");
    
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();
    
    if(curl) {
        cJSON *sym = NULL;
        cJSON_ArrayForEach(sym, symbols) {
            cJSON *tf = NULL;
            cJSON_ArrayForEach(tf, timeframes) {
                char *s = sym->valuestring;
                char clean_sym[64];
                int j = 0;
                for(int i=0; s[i]; i++) {
                    if(s[i] != '/') clean_sym[j++] = s[i];
                }
                clean_sym[j] = '\0';
                
                char url[512];
                snprintf(url, sizeof(url), "https://%s%s?symbol=%s&interval=%s&limit=100", 
                         app_config.binance_rest_host, app_config.binance_rest_path, clean_sym, tf->valuestring);
                
                struct MemoryStruct chunk;
                chunk.memory = malloc(1);  
                chunk.size = 0;
                
                curl_easy_setopt(curl, CURLOPT_URL, url);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
                curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
                
                CURLcode res = curl_easy_perform(curl);
                
                if(res != CURLE_OK) {
                    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                } else {
                    printf("%lu bytes retrieved for %s %s\n", (unsigned long)chunk.size, clean_sym, tf->valuestring);
                    
                    cJSON *json = cJSON_Parse(chunk.memory);
                    if (json && cJSON_IsArray(json)) {
                        int count = cJSON_GetArraySize(json);
                        Candle *candles = malloc(sizeof(Candle) * count);
                        int idx = 0;
                        cJSON *item = NULL;
                        cJSON_ArrayForEach(item, json) {
                            if (cJSON_GetArraySize(item) >= 7) {
                                candles[idx].t = (double)cJSON_GetArrayItem(item, 0)->valuedouble; 
                                candles[idx].o = atof(cJSON_GetArrayItem(item, 1)->valuestring);
                                candles[idx].h = atof(cJSON_GetArrayItem(item, 2)->valuestring);
                                candles[idx].l = atof(cJSON_GetArrayItem(item, 3)->valuestring);
                                candles[idx].c = atof(cJSON_GetArrayItem(item, 4)->valuestring);
                                candles[idx].v = atof(cJSON_GetArrayItem(item, 5)->valuestring);
                                candles[idx].T = (double)cJSON_GetArrayItem(item, 6)->valuedouble; 
                                
                                strncpy(candles[idx].s, clean_sym, sizeof(candles[idx].s)-1);
                                strncpy(candles[idx].i, tf->valuestring, sizeof(candles[idx].i)-1);
                                
                                idx++;
                            }
                        }
                        
                        char stream_name[128];
                        int k = 0;
                        for(int i=0; clean_sym[i]; i++) {
                            stream_name[k++] = tolower(clean_sym[i]);
                        }
                        stream_name[k] = '\0';
                        snprintf(stream_name + k, sizeof(stream_name) - k, "@kline_%s", tf->valuestring);
                        
                        storage_push_history(stream_name, candles, idx);
                        
                        free(candles);
                        cJSON_Delete(json);
                    } else {
                        printf("Failed to parse JSON for %s\n", url);
                    }
                }
                
                free(chunk.memory);
            }
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

static char *get_subscribe_payload(const char *target_tf) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "method", "SUBSCRIBE");
    cJSON *params = cJSON_CreateArray();
    
    cJSON *symbols = cJSON_GetObjectItem(app_config.json, "symbols");
    
    cJSON *sym = NULL;
    cJSON_ArrayForEach(sym, symbols) {
        char stream[128];
        // Convert BTC/USDT to btcusdt
        char *s = sym->valuestring;
        char clean_sym[64];
        int j = 0;
        for(int i=0; s[i]; i++) {
            if(s[i] != '/') clean_sym[j++] = tolower(s[i]);
        }
        clean_sym[j] = '\0';
        
        snprintf(stream, sizeof(stream), "%s@kline_%s", clean_sym, target_tf);
        cJSON_AddItemToArray(params, cJSON_CreateString(stream));
    }
    
    cJSON_AddItemToObject(root, "params", params);
    cJSON_AddNumberToObject(root, "id", 1);
    
    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return payload;
}

int callback_binance(struct lws *wsi, enum lws_callback_reasons reason,
                     void *user, void *in, size_t len) {
    binance_client_t *client = (binance_client_t *)user;
    (void)len;
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            lwsl_user("Binance Connected (%s)\n", client ? client->timeframe : "?");
            if (client) client->wsi = wsi;
            lws_callback_on_writable(wsi);
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            if (client) {
                char *payload = get_subscribe_payload(client->timeframe);
                size_t n = strlen(payload);
                unsigned char *buf = malloc(LWS_PRE + n);
                memcpy(&buf[LWS_PRE], payload, n);
                lws_write(wsi, &buf[LWS_PRE], n, LWS_WRITE_TEXT);
                free(buf);
                free(payload);
            }
            break;
        }

        case LWS_CALLBACK_CLIENT_RECEIVE: {
            // Fast path: Check if candle is closed using AVX2
            // We only care about "kline" events which have "e":"kline" and "k":{...}
            // But checking "x":true is usually enough filter for closed candles in the stream
            
            // First, verify it's a kline event (optional but safer)
            // For max speed we could skip this if we trust the stream
            // But let's do a quick check for "kline"
            if (!strstr((char*)in, "kline")) break;

            // Check if closed
            if (!is_candle_closed_avx2((char*)in)) break;

            // Parse candle
            Candle candle;
            memset(&candle, 0, sizeof(Candle));
            
            // Find "k": object start
            const char* kptr = strstr((char*)in, "\"k\":");
            if (!kptr) break;
            kptr += 4; // skip "k":

            parse_candle_avx2(kptr, &candle);

            // Construct stream name to find buffer
            if (candle.s[0] && candle.i[0]) {
                char clean_sym[64];
                int j = 0;
                for(int idx=0; candle.s[idx]; idx++) {
                    if(candle.s[idx] != '/') clean_sym[j++] = tolower(candle.s[idx]);
                }
                clean_sym[j] = '\0';
                char stream_name[128];
                snprintf(stream_name, sizeof(stream_name), "%s@kline_%s", clean_sym, candle.i);
                
                storage_push(stream_name, candle);

                char candle_json[1024];
                snprintf(candle_json, sizeof(candle_json),
                    "{\"t\":%.0f,\"T\":%.0f,\"s\":\"%s\",\"i\":\"%s\","
                    "\"o\":\"%.8f\",\"c\":\"%.8f\",\"h\":\"%.8f\",\"l\":\"%.8f\",\"v\":\"%.8f\"}",
                    candle.t, candle.T, candle.s, candle.i,
                    candle.o, candle.c, candle.h, candle.l, candle.v);

                // Calculate indicators
                CircularBuffer *buf_storage = get_buffer(stream_name);
                if (buf_storage && buf_storage->count > 30) { // Need enough data for indicators
                    Candle *linear_candles = malloc(sizeof(Candle) * buf_storage->count);
                    size_t count = circular_to_linear(buf_storage, linear_candles);
                    
                    cJSON *ind_json = calculate_indicators(linear_candles, count);
                    
                    if (ind_json) {
                        char *indicators_str = cJSON_PrintUnformatted(ind_json);
                        
                        // Create combined message manually
                        size_t combined_len = strlen(stream_name) + strlen(candle_json) + strlen(indicators_str) + 64;
                        char *combined_str = malloc(combined_len);
                        
                        snprintf(combined_str, combined_len,
                            "{\"stream\":\"%s\",\"candle\":%s,\"indicators\":%s}",
                            stream_name, candle_json, indicators_str);
                        
                        broadcast_msg(combined_str);
                        
                        free(combined_str);
                        free(indicators_str);
                        cJSON_Delete(ind_json);
                    } else {
                        broadcast_msg(candle_json);
                    }
                    free(linear_candles);
                } else {
                    broadcast_msg(candle_json);
                }
            }
            break;
        }

        case LWS_CALLBACK_CLIENT_CLOSED:
            if (client) client->wsi = NULL;
            lwsl_warn("Binance Closed (%s)\n", client ? client->timeframe : "?");
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            if (client) client->wsi = NULL;
            lwsl_err("Binance Connection Error (%s)\n", client ? client->timeframe : "?");
            break;

        default:
            break;
    }
    return 0;
}
