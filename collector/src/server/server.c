#include "server.h"
#include <stdlib.h>
#include <string.h>

static struct per_session_data__server *client_list = NULL;

void broadcast_msg(const char *msg) {
    // printf("%s\n",msg);
    struct per_session_data__server *pss = client_list;
    while (pss) {
        mq_push(&pss->msg_q_head, &pss->msg_q_tail, msg);
        lws_callback_on_writable(pss->wsi);
        pss = pss->next;
    }
}

int callback_server(struct lws *wsi, enum lws_callback_reasons reason,
                    void *user, void *in, size_t len) {
    struct per_session_data__server *pss = (struct per_session_data__server *)user;
    (void)in;
    (void)len;

    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            lwsl_user("Client Connected\n");
            pss->wsi = wsi;
            pss->msg_q_head = NULL;
            pss->msg_q_tail = NULL;
            
            pss->next = client_list;
            client_list = pss;
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE: {
            char *msg = mq_pop(&pss->msg_q_head, &pss->msg_q_tail);
            if (msg) {
                size_t n = strlen(msg);
                unsigned char *buf = malloc(LWS_PRE + n);
                memcpy(&buf[LWS_PRE], msg, n);
                lws_write(wsi, &buf[LWS_PRE], n, LWS_WRITE_TEXT);
                free(buf);
                free(msg);
                
                if (pss->msg_q_head) {
                    lws_callback_on_writable(wsi);
                }
            }
            break;
        }

        case LWS_CALLBACK_CLOSED:
            lwsl_warn("Client Closed\n");
            if (client_list == pss) {
                client_list = pss->next;
            } else {
                struct per_session_data__server *curr = client_list;
                while (curr && curr->next != pss) {
                    curr = curr->next;
                }
                if (curr) {
                    curr->next = pss->next;
                }
            }
            mq_free_all(&pss->msg_q_head, &pss->msg_q_tail);
            break;

        default:
            break;
    }
    return 0;
}
