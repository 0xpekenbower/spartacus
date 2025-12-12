#ifndef SERVER_H
#define SERVER_H

#include <libwebsockets.h>
#include "../utils/queue.h"

struct per_session_data__server {
    struct per_session_data__server *next;
    struct lws *wsi;
    struct msg_queue *msg_q_head;
    struct msg_queue *msg_q_tail;
};

int callback_server(struct lws *wsi, enum lws_callback_reasons reason,
                    void *user, void *in, size_t len);

void broadcast_msg(const char *msg);

#endif
