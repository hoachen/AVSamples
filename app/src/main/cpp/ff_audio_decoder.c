//
// Created by chenhao on 2021/5/17.
//

#include "ff_audio_decoder.h"
#include "log.h"


Decoder *ff_decoder_create()
{
    Decoder *decoder = av_mallocz(sizeof(Decoder));
    if (decoder) {
        decoder->stream_index = -1;
        decoder->read_eof = 0;
        decoder->has_flush_decoder = 0;
    }
    return decoder;
}

int ff_decoder_init(Decoder *decoder, const char *input_file_path, int channel, int sample_rate)
{
    int ret = 0;
    unsigned int i;
    int64_t dst_channel_layout = AV_CH_LAYOUT_STEREO;
    dst_channel_layout = (channel == 2) ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;
    int dst_sample_rate = 44100;
    dst_sample_rate = sample_rate;
    if ((ret = avformat_open_input(&decoder->ifmt_ctx, input_file_path, NULL, NULL)) < 0) {
        LOGE("open input failed");
        return ret;
    }
    if ((ret = avformat_find_stream_info(decoder->ifmt_ctx, NULL)) < 0) {
        LOGE("find stream info error");
    }
    for (i = 0 ; i < decoder->ifmt_ctx->nb_streams; i++) {
        AVStream *stream = decoder->ifmt_ctx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            decoder->stream_index = i;
            AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
            if (!dec) {
                LOGE("Decoder no found");
                return AVERROR_INVALIDDATA;
            }
            decoder->dec_ctx = avcodec_alloc_context3(dec);
            if (!decoder->dec_ctx) {
                LOGE("alloc decoder context found");
                return AVERROR(ENOMEM);
            }
            ret = avcodec_parameters_to_context(decoder->dec_ctx, stream->codecpar);
            if (ret < 0) {
                LOGE("avcodec_parameters_to_context failed");
                return ret;
            }
            ret = avcodec_open2(decoder->dec_ctx, dec, NULL);
            if (ret < 0) {
                LOGE("open decoder failed");
                return ret;
            }
            break;
        }
    }
    if (decoder->stream_index >= 0 && decoder->dec_ctx) {
        if (dst_channel_layout != decoder->dec_ctx->channel_layout
        || dst_sample_rate != decoder->dec_ctx->sample_rate) {
            decoder->swr_ctx = swr_alloc();
            if (!decoder->swr_ctx) {
                LOGE("alloc swr context failed");
                return -1;
            }
            av_opt_set_int(decoder->swr_ctx, "in_channel_layout", decoder->dec_ctx->channel_layout, 0);
            av_opt_set_int(decoder->swr_ctx, "in_sample_rate",    decoder->dec_ctx->channel_layout, 0);
            av_opt_set_sample_fmt(decoder->swr_ctx, "in_sample_fmt", decoder->dec_ctx->sample_fmt, 0);
            av_opt_set_int(decoder->swr_ctx, "out_channel_layout",    dst_channel_layout, 0);
            av_opt_set_int(decoder->swr_ctx, "out_sample_rate",       dst_sample_rate, 0);
            av_opt_set_sample_fmt(decoder->swr_ctx, "out_sample_fmt", decoder->dec_ctx->sample_fmt, 0);
            if ((ret = swr_init(decoder->swr_ctx)) < 0) {
                LOGE("Failed to initialize the resampling context\n");
                return -1;
            }
        }
    }
    return 0;
}

int ff_decoder_decode(Decoder *decoder, uint8_t **buffer, int *size, int64_t *pts)
{
    int ret = 0;
    AVPacket pkt;
    if (decoder->ifmt_ctx && !decoder->read_eof) {
        av_init_packet(&pkt);
        while ((ret = av_read_frame(decoder->ifmt_ctx, &pkt)) >= 0) {
            if (decoder->stream_index == pkt.stream_index) {
                ret = avcodec_send_frame(decoder->dec_ctx, )
            }
            av_packet_unref(&pkt);
        }
    }
    if (!decoder->has_flush_decoder) {

    }
}

int ff_decoder_release(Decoder *decoder);
