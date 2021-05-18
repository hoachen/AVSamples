//
// Created by chenhao on 2021/5/17.
//

#ifndef AVSAMPLES_FF_AUDIO_DECODER_H
#define AVSAMPLES_FF_AUDIO_DECODER_H

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"

typedef struct Decoder {
    AVFormatContext *ifmt_ctx;
    AVCodecContext *dec_ctx;
    SwrContext *swr_ctx;
    int stream_index;
    int read_eof;
    int decode_eof;
    int64_t dst_ch_layout;
    int dst_sample_rate;
    int max_dst_nb_samples;
    int dst_nb_channels;
    int dst_linesize;
    uint8_t **dst_data;
} Decoder;


Decoder *ff_decoder_create();

int ff_decoder_init(Decoder *decoder, const char *input_file_path, int channel, int sample_rate);

int ff_decoder_decode(Decoder *decoder, uint8_t **buffer, int *size, int64_t *pts);

int ff_decoder_release(Decoder *decoder);

#endif //AVSAMPLES_FF_AUDIO_DECODER_H
