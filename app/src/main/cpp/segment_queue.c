//
// Created by chenhao on 1/1/21.
//

#include "segment_queue.h"

int segment_queue_init(SegmentQueue *q, int size)
{
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
    q->segments = (Segment *)av_mallocz(size * sizeof(Segment));
    if (!q->segments) {
        return -1;
    }
    q->rindex = 0;
    q->windex = 0;
    q->size = size;
    q->abort_request = 0;
    return 0;
}

int segment_queue_abort(SegmentQueue *q)
{
    pthread_mutex_lock(&q->mutex);
    q->abort_request = 1;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}


void segment_queue_push(SegmentQueue *q)
{
    pthread_mutex_lock(&q->mutex);
    if (++q->windex >= q->size) {
        q->windex = 0;
    }
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

Segment *segment_queue_peek_writable(SegmentQueue *q)
{
    Segment *segment;
    pthread_mutex_lock(&q->mutex);
    if (q->abort_request)
        return NULL;
    segment = q->segments + q->windex;
    pthread_mutex_unlock(&q->mutex);
    return segment;
}

void segment_queue_next(SegmentQueue *q)
{
    pthread_mutex_lock(&q->mutex);
    if (++q->rindex >= q->size) {
        q->rindex = 0;
    }
    pthread_mutex_unlock(&q->mutex);
}

Segment *segment_queue_peek_readable(SegmentQueue *q)
{
    Segment *segment;
    pthread_mutex_lock(&q->mutex);
    segment = q->segments + q->rindex;
    while (!segment->exist && !q->abort_request) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }
    pthread_mutex_unlock(&q->mutex);
    if (q->abort_request)
        return NULL;
    return segment;
}

int segment_queue_destroy(SegmentQueue *q)
{
    if (q->segments)
        free(q->segments);
    pthread_cond_destroy(&q->cond);
    pthread_mutex_destroy(&q->mutex);
}