//
// Created by chenhao on 1/6/21.
//

#include "message_queue.h"
#include "msg_def.h"

int msg_queue_init(MessageQueue *q)
{
    memset(q, 0, sizeof(MessageQueue));
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
    q->abort_request = 1;
    q->nb_messages = 0;
    q->first_msg = NULL;
    q->last_msg = NULL;
    return 0;
}

inline static void msg_init_msg(AVMessage *msg)
{
    memset(msg, 0, sizeof(AVMessage));
}

static int msg_queue_put_private(MessageQueue *q, AVMessage *msg)
{
    AVMessage *msg1;
    msg1 = malloc(sizeof(AVMessage));
    if (!msg1)
        return -1;
    *msg1 = *msg;
    msg1->next = NULL;
    if (q->abort_request)
        return -1;
    if (!q->last_msg) {
        q->first_msg = msg1;
    } else {
        q->last_msg->next = msg1;
    }
    q->last_msg = msg1;
    q->nb_messages++;
}

int msg_queue_put(MessageQueue *q, AVMessage *msg)
{
    pthread_mutex_lock(&q->mutex);
    msg_queue_put_private(q, msg);
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

int msg_queue_put_simple1(MessageQueue *q, int what)
{
    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = what;
    msg_queue_put(q, &msg);
}

int msg_queue_put_simple2(MessageQueue *q, int what, int arg1)
{
    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = what;
    msg.arg1 = arg1;
    msg_queue_put(q, &msg);
}

int msg_queue_put_simple3(MessageQueue *q, int what, int arg1, int arg2)
{
    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = what;
    msg.arg1 = arg1;
    msg.arg2 = arg2;
    msg_queue_put(q, &msg);
}

int msg_queue_put_simple4(MessageQueue *q, int what, int arg1, int arg2, void *obj, int obj_len)
{
    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = what;
    msg.arg1 = arg1;
    msg.arg2 = arg2;
    msg.obj = malloc(obj_len);
    memcpy(msg.obj, obj, obj_len);
    msg_queue_put(q, &msg);
}

int msg_queue_get(MessageQueue *q, AVMessage *msg, int block)
{
    int ret;
    AVMessage  *msg1;
    pthread_mutex_lock(&q->mutex);
    for (;;) {
        if (q->abort_request) {
            ret = -1;
            break;
        }
        if (q->first_msg) {
            msg1 = q->first_msg;
            q->first_msg = msg1->next;
            if (!q->first_msg) {
                q->last_msg = q->first_msg;
            }
            *msg = *msg1;
            msg1->obj = NULL;
            free(msg1);
            q->nb_messages--;
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            pthread_cond_wait(&q->cond, &q->mutex);
        }
    }
    pthread_mutex_unlock(&q->mutex);
    return ret;
}


int msg_queue_start(MessageQueue *q)
{
    pthread_mutex_lock(&q->mutex);
    q->abort_request = 0;
    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = MSG_FLUSH;
    msg_queue_put_private(q, &msg);
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

int msg_queue_abort(MessageQueue *q)
{
    pthread_mutex_lock(&q->mutex);
    q->abort_request = 1;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

int msg_queue_remove(MessageQueue *q, int what)
{
    AVMessage **p_msg, *msg, *last_msg;
    pthread_mutex_lock(&q->mutex);
    last_msg = q->first_msg;
    if (!q->abort_request && q->first_msg) {
        *p_msg = q->first_msg;
        while(*p_msg) {
            msg = *p_msg;
            if (msg->what == what) {
                // 删除此节点
                *p_msg = msg->next;
                free(msg);
                q->nb_messages--;
            } else {
                last_msg = msg;
                p_msg = &msg->next;
            }
        }
        if (q->first_msg) {
            q->last_msg = last_msg;
        } else {
            q->last_msg = NULL;
        }
    }
    pthread_mutex_unlock(&q->mutex);
}

int msg_queue_count(MessageQueue *q, int what)
{
    AVMessage *msg, *msg1;
    int count = 0;
    pthread_mutex_lock(&q->mutex);
    if (!q->abort_request && q->first_msg) {
        for (msg = q->first_msg; msg != NULL; msg = msg1) {
            msg1 = msg->next;
            if (msg->what == what)
                count++;
        }
    }
    pthread_mutex_unlock(&q->mutex);
    return count;
}

int msg_queue_flush(MessageQueue *q)
{
    AVMessage *msg, *msg1;
    pthread_mutex_lock(&q->mutex);
    for (msg = q->first_msg; msg != NULL; msg = msg1) {
        msg1 = msg->next;
        free(msg);
    }
    q->first_msg = NULL;
    q->last_msg = NULL;
    q->nb_messages = 0;
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

int msg_queue_destroy(MessageQueue *q)
{
    msg_queue_flush(q);
    pthread_cond_destroy(&q->cond);
    pthread_mutex_destroy(&q->mutex);
}