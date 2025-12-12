#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <libwebsockets.h>

AppConfig app_config = {
    .binance_host = "fstream.binance.com",
    .binance_path = "/ws",
    .binance_rest_host = "fapi.binance.com",
    .binance_rest_path = "/fapi/v1/klines",
    .listen_port = 8080,
    .buffer_size = 1000,
    .json = NULL
};

int load_config(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        lwsl_err("Failed to open %s\n", filename);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc(len + 1);
    fread(data, 1, len, f);
    data[len] = '\0';
    fclose(f);
    
    app_config.json = cJSON_Parse(data);
    free(data);
    if (!app_config.json) return -1;

    cJSON *j_host = cJSON_GetObjectItem(app_config.json, "binance_url");
    if (j_host) app_config.binance_host = j_host->valuestring;
    
    cJSON *j_port = cJSON_GetObjectItem(app_config.json, "listen_port");
    if (j_port) app_config.listen_port = j_port->valueint;

    cJSON *j_buf = cJSON_GetObjectItem(app_config.json, "buffer_size");
    if (j_buf) app_config.buffer_size = j_buf->valueint;

    return 0;
}

void free_config() {
    if (app_config.json) {
        cJSON_Delete(app_config.json);
        app_config.json = NULL;
    }
}
