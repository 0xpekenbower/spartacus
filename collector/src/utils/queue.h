#ifndef QUEUE_H
#define QUEUE_H

struct msg_queue {
    char *payload;
    struct msg_queue *next;
};

void mq_push(struct msg_queue **head, struct msg_queue **tail, const char *msg);
char *mq_pop(struct msg_queue **head, struct msg_queue **tail);
void mq_free_all(struct msg_queue **head, struct msg_queue **tail);

#endif
