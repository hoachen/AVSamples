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

typedef struct GopSegment
{
    char mp4_path[1024];
    char yuv_path[1024];
    int width;
    int height;
    int frames;
    int exist;
    int64_t start_time;
    int64_t frame_show_time_ms;
    int64_t duration;
} GopSegment;


typedef struct SegmentQueue
{
    GopSegment *segments;
    int size;
    int rindex;
    int windex;
    int64_t duration;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int abort_request;
} SegmentQueue;

int segment_queue_init(SegmentQueue *q, int size);

void segment_queue_push(SegmentQueue *q);

GopSegment *segment_queue_peek_writable(SegmentQueue *q);

void segment_queue_next(SegmentQueue *q);

GopSegment *segment_queue_peek_readable(SegmentQueue *q);


int segment_queue_abort(SegmentQueue *q);

int segment_queue_destroy(SegmentQueue *q);


#endif //AVSAMPLES_SEGMENT_QUEUE_H