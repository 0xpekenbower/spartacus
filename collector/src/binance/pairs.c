#include "pairs.h"
#include "../config/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

struct MemoryStruct {
    char *memory;
    size_t size;
};

typedef struct {
    char symbol[32];
    double priceChangePercent;
    double quoteVolume;
    double lastPrice;
} PairInfo;

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(!ptr) return 0;
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

static int compare_pairs(const void *a, const void *b) {
    const PairInfo *pa = (const PairInfo *)a;
    const PairInfo *pb = (const PairInfo *)b;
    // Sort descending by priceChangePercent
    if (pb->priceChangePercent > pa->priceChangePercent) return 1;
    if (pb->priceChangePercent < pa->priceChangePercent) return -1;
    return 0;
}

int update_config_with_top_pairs() {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (!curl) {
        free(chunk.memory);
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, "https://fapi.binance.com/fapi/v1/ticker/24hr");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(chunk.memory);
        curl_easy_cleanup(curl);
        return -1;
    }

    cJSON *json = cJSON_Parse(chunk.memory);
    if (!json) {
        fprintf(stderr, "JSON parse error\n");
        free(chunk.memory);
        curl_easy_cleanup(curl);
        return -1;
    }

    PairInfo *pairs = NULL;
    size_t count = 0;
    size_t capacity = 0;

    cJSON *item;
    cJSON_ArrayForEach(item, json) {
        cJSON *symbol = cJSON_GetObjectItem(item, "symbol");
        cJSON *lastPrice = cJSON_GetObjectItem(item, "lastPrice");
        cJSON *quoteVolume = cJSON_GetObjectItem(item, "quoteVolume");
        cJSON *priceChangePercent = cJSON_GetObjectItem(item, "priceChangePercent");

        if (symbol && lastPrice && quoteVolume && priceChangePercent) {
            if (strstr(symbol->valuestring, "USDT") == NULL) continue;
            double price = atof(lastPrice->valuestring);
            if (price <= 5.0) continue;
            double volume = atof(quoteVolume->valuestring);
            if (volume < 10000000.0) continue;

            if (count >= capacity) {
                capacity = capacity == 0 ? 128 : capacity * 2;
                pairs = realloc(pairs, sizeof(PairInfo) * capacity);
            }

            strncpy(pairs[count].symbol, symbol->valuestring, 31);
            pairs[count].symbol[31] = '\0';
            pairs[count].lastPrice = price;
            pairs[count].quoteVolume = volume;
            pairs[count].priceChangePercent = atof(priceChangePercent->valuestring);
            count++;
        }
    }

    qsort(pairs, count, sizeof(PairInfo), compare_pairs);
    size_t top_n = count < 50 ? count : 50;
    
    printf("Selected Top %zu pairs:\n", top_n);
    
    cJSON *new_symbols = cJSON_CreateArray();
    for (size_t i = 0; i < top_n; i++) {
        printf("%d. %s (Price: %.2f, Change: %.2f%%, Vol: %.0f)\n", 
               (int)i+1, pairs[i].symbol, pairs[i].lastPrice, pairs[i].priceChangePercent, pairs[i].quoteVolume);
        cJSON_AddItemToArray(new_symbols, cJSON_CreateString(pairs[i].symbol));
    }
    if (app_config.json) {
        cJSON_DeleteItemFromObject(app_config.json, "symbols");
        cJSON_AddItemToObject(app_config.json, "symbols", new_symbols);
    }

    free(pairs);
    cJSON_Delete(json);
    free(chunk.memory);
    curl_easy_cleanup(curl);
    
    return 0;
}
