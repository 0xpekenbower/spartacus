#include "queue.h"
#include <stdlib.h>
#include <string.h>

void mq_push(struct msg_queue **head, struct msg_queue **tail, const char *msg) {
    struct msg_queue *node = malloc(sizeof(struct msg_queue));
    node->payload = strdup(msg);
    node->next = NULL;
    if (*tail) {
        (*tail)->next = node;
        *tail = node;
    } else {
        *head = *tail = node;
    }
}

char *mq_pop(struct msg_queue **head, struct msg_queue **tail) {
    if (!*head) return NULL;
    struct msg_queue *node = *head;
    char *msg = node->payload;
    *head = node->next;
    if (!*head) *tail = NULL;
    free(node);
    return msg;
}

void mq_free_all(struct msg_queue **head, struct msg_queue **tail) {
    while (*head) {
        char *msg = mq_pop(head, tail);
        free(msg);
    }
}
