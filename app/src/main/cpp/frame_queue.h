//
// Created by chenhao on 12/30/20.
//

#ifndef AVSAMPLES_FRAME_QUEUE_H
#define AVSAMPLES_FRAME_QUEUE_H

#include <libavformat/avformat.h>
#include <libav>


typedef struct FrameQueue {
    AVFrame *frame;
} FrameQueue;

#endif //AVSAMPLES_FRAME_QUEUE_H
