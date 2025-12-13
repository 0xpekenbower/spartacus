#ifndef BINANCE_H
#define BINANCE_H

#include <libwebsockets.h>

// extern struct lws *wsi_binance;

typedef struct {
    struct lws *wsi;
    char timeframe[16];
} binance_client_t;

int callback_binance(struct lws *wsi, enum lws_callback_reasons reason,
                     void *user, void *in, size_t len);

void fetch_historical_data(struct lws_context *context);

#endif
