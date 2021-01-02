//
// Created by chenhao on 1/1/21.
//

#ifndef AVSAMPLES_SEGMENT_QUEUE_H
#define AVSAMPLES_SEGMENT_QUEUE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <libavutil/avutil.h>
#include "log.h"

typedef struct YUVSegment
{
    char *path;
    int width;
    int height;
    int frames;
    int64_t frame_show_time_ms;
    int64_t duration;
    struct YUVSegment *next;
} YUVSegment;

typedef struct SegmentQueue
{
    YUVSegment *head, *tail;
    int size;
    int64_t frame_count;
    int64_t duration;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int abort_request;
} SegmentQueue;

int segment_queue_init(SegmentQueue *q);

int segment_queue_abort(SegmentQueue *q);

int segment_queue_put(SegmentQueue *q, char *yuv_path, int width, int height, int nb_frames, int64_t duration, int64_t frame_show_time);

int segment_queue_get(SegmentQueue *q, YUVSegment *segment, int block);

int segment_queue_flush(SegmentQueue *q);

int segment_queue_destroy(SegmentQueue *q);


#endif //AVSAMPLES_SEGMENT_QUEUE_H