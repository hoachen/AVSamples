//
// Created by chenhao on 1/6/21.
//

#ifndef AVSAMPLES_MESSAGE_QUEUE_H
#define AVSAMPLES_MESSAGE_QUEUE_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct AVMessage {
    int what;
    int arg1;
    int arg2;
    void *obj;
    struct AVMessage *next;
} AVMessage;

typedef struct MessageQueue {
    AVMessage *first_msg, *last_msg;
    int nb_messages;
    int abort_request;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} MessageQueue;

int msg_queue_init(MessageQueue *q);

int msg_queue_put(MessageQueue *q, AVMessage *msg);

int msg_queue_put_simple1(MessageQueue *q, int what);

int msg_queue_put_simple2(MessageQueue *q, int what, int arg1);

int msg_queue_put_simple3(MessageQueue *q, int what, int arg1, int arg2);

int msg_queue_put_simple4(MessageQueue *q, int what, int arg1, int arg2, void *obj, int obj_len);

int msg_queue_get(MessageQueue *q, AVMessage *msg, int block);

int msg_queue_remove(MessageQueue *q, int what);

int msg_queue_count(MessageQueue *q, int what);

int msg_queue_start(MessageQueue *q);

int msg_queue_abort(MessageQueue *q);

int msg_queue_flush(MessageQueue *q);

int msg_queue_destroy(MessageQueue *q);

#endif //AVSAMPLES_MESSAGE_QUEUE_H
