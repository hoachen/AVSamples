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

typedef struct Segment
{
    char mp4_path[1024];
    char yuv_path[1024];
    int width;
    int height;
    int frames;
    int exist;
    int index;
    int64_t start_time;
    int64_t frame_show_time_ms;
    int64_t duration;
} Segment;

typedef struct SegmentNode {
    struct Segment segment;
    struct SegmentNode *next;
} SegmentNode;

typedef struct SegmentQueue
{
    SegmentNode *head, *tail;
    int size;
    int64_t frame_count;
    int64_t duration;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int abort_request;
} SegmentQueue;

int segment_queue_init(SegmentQueue *q);

int segment_queue_abort(SegmentQueue *q);

int segment_queue_put(SegmentQueue *q, Segment *segment);

int segment_queue_get(SegmentQueue *q, Segment *segment, int block);

int segment_queue_flush(SegmentQueue *q);

int segment_queue_destroy(SegmentQueue *q);

#endif //AVSAMPLES_SEGMENT_QUEUE_H