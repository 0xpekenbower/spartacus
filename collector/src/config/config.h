#ifndef CONFIG_H
#define CONFIG_H

#include <cjson/cJSON.h>

typedef struct {
    char *binance_host;
    char *binance_path;
    char *binance_rest_host;
    char *binance_rest_path;
    int listen_port;
    int buffer_size;
    cJSON *json;
} AppConfig;

extern AppConfig app_config;

int load_config(const char *filename);
void free_config();

#endif
