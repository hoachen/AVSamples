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

int segment_queue_put_l(SegmentQueue *q, char *yuv_path, int width, int height, int frames, int64_t duration, int64_t frame_show_time)
{
    if (q->abort_request)
        return -1;
    YUVSegment *segment = (YUVSegment *)malloc(sizeof(YUVSegment));
    if (!segment) {
        LOGE("malloc YUVSegment failed");
        return -1;
    }
    memset(segment, 0, sizeof(YUVSegment));
    segment->path = strdup(yuv_path);
    segment->width = width;
    segment->height = height;
    segment->frames = frames;
    segment->duration = duration;
    segment->frame_show_time_ms = frame_show_time;
    segment->next = NULL;
    if (q->tail) {
        q->tail->next = segment;
    }
    q->tail = segment;
    if (!q->head) {
        q->head = q->tail;
    }
    q->duration += duration;
    q->size ++;
    q->frame_count += frames;

    pthread_cond_signal(&q->cond);
    return 0;
}

int segment_queue_put(SegmentQueue *q, char *yuv_path, int width, int height, int frames, int64_t duration, int64_t frame_show_time_ms)
{
    int ret = 0;
    pthread_mutex_lock(&q->mutex);
    ret = segment_queue_put_l(q, yuv_path, width, height, frames, duration, frame_show_time_ms);
    pthread_mutex_unlock(&q->mutex);
    return ret;
}

int segment_queue_get(SegmentQueue *q, YUVSegment *segment, int block)
{
    int ret;
    YUVSegment *s1 = NULL;
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
            *segment = *s1;
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
    YUVSegment *s1, *s2;
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