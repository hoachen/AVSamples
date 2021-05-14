//
// Created by chenhao on 2021/5/13.
//

#ifndef AVSAMPLES_TRANSCODE_H
#define AVSAMPLES_TRANSCODE_H

#include <stdio.h>
#include <libavutil/opt.h>
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavcodec/avcodec.h"
#include "log.h"

typedef struct StreamContext {
    AVCodecContext *dec_ctx;
    AVCodecContext *enc_ctx;
    AVFrame *dec_frame;
    AVFrame *enc_frame;
} StreamContext;

typedef struct Transcoder_ {
    int video_width;
    int video_height;
    int fps;
    int video_bitrate;
    int dst_sample_rate;
    int64_t dst_channel_layout;
    enum AVSampleFormat dst_sample_fmt;
    int audio_bitrate;
    char input_file[1024];
    char output_file[1024];
    AVFormatContext *ifmt_ctx;
    AVFormatContext *ofmt_ctx;
    StreamContext *stream_ctx;
    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;
    int video_stream_idx;
    int audio_stream_idx;
    int64_t output_video_pts;
    int64_t output_audio_pts;
} Transcoder;

Transcoder *create_transcoder();

int transcode_init(Transcoder *transcoder, const char *input_file, const char *output_file,
         int width, int height, int fps, int v_bitrate,
         int sample_rate, int channel, int a_bitrate);

int transcode_start(Transcoder *transcoder);

int transcode_stop(Transcoder *transcoder);

void transcode_destroy(Transcoder *transcoder);

#endif //AVSAMPLES_TRANSCODE_H
