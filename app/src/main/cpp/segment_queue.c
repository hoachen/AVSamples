//
// Created by chenhao on 1/1/21.
//

#include "segment_queue.h"

int segment_queue_init(SegmentQueue *q)
{
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
    q->head = NULL;
    q->tail = NULL;
    q->duration = 0;
    q->frame_count = 0;
    q->size = 0;
    q->abort_request = 0;
}

int segment_queue_abort(SegmentQueue *q)
{
    pthread_mutex_lock(&q->mutex);
    q->abort_request = 1;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

int segment_queue_put_l(SegmentQueue *q, Segment *segment)
{
    if (q->abort_request)
        return -1;
    SegmentNode *s1 = (SegmentNode *)malloc(sizeof(SegmentNode));
    if (!segment) {
        LOGE("malloc SegmentNode failed");
        return -1;
    }
    memset(s1, 0, sizeof(SegmentNode));
    s1->next = NULL;
    s1->segment = *segment;

    if (q->tail) {
        q->tail->next = s1;
    }
    q->tail = s1;
    if (!q->head) {
        q->head = q->tail;
    }
    q->duration += segment->duration;
    q->size ++;
    q->frame_count += segment->frames;
    pthread_cond_signal(&q->cond);
    return 0;
}

int segment_queue_put(SegmentQueue *q, Segment *segment)
{
    int ret = 0;
    pthread_mutex_lock(&q->mutex);
    ret = segment_queue_put_l(q, segment);
    pthread_mutex_unlock(&q->mutex);
    return ret;
}

int segment_queue_get(SegmentQueue *q, Segment *segment, int block)
{
    int ret;
    SegmentNode *s1 = NULL;
    pthread_mutex_lock(&q->mutex);
    for (;;) {
        if (q->abort_request) {
            ret = -1;
            break;
        }
        if (q->head) {
            s1 = q->head;
            q->head = s1->next;
            if (!q->head) {
                q->tail = NULL;
            }
            ret = 1;
            *segment = s1->segment;
            q->size--;
            q->duration -= segment->duration;
            q->frame_count -= segment->frames;
            // !! free();
            free(s1);
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

int segment_queue_flush(SegmentQueue *q)
{
    SegmentNode *s1, *s2;
    pthread_mutex_unlock(&q->mutex);
    for (s1 = q->head; s1; s1 = s2) {
        s2 = s1->next;
        free(s1);
    }
    q->head = NULL;
    q->tail = NULL;
    q->frame_count = 0;
    q->size = 0;
    q->duration = 0;
    pthread_mutex_unlock(&q->mutex);
}

int segment_queue_destroy(SegmentQueue *q)
{
    segment_queue_flush(q);
    pthread_cond_destroy(&q->cond);
    pthread_mutex_destroy(&q->mutex);
}