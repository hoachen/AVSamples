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
        decoder->decode_eof= 0;
    }
    return decoder;
}

int ff_decoder_init(Decoder *decoder, const char *input_file_path, int channel, int sample_rate)
{
    int ret = 0;
    unsigned int i;
    decoder->dst_ch_layout = (channel == 2) ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;
    decoder->dst_sample_rate = sample_rate;
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

            decoder->max_dst_nb_samples = av_rescale_rnd( stream->codecpar->frame_size, decoder->dst_sample_rate,
                                   decoder->dst_sample_rate, AV_ROUND_UP);

            /* buffer is going to be directly written to a rawaudio file, no alignment */
            decoder->dst_nb_channels = av_get_channel_layout_nb_channels(decoder->dst_ch_layout);
            ret = av_samples_alloc_array_and_samples(&decoder->dst_data, &decoder->dst_linesize, decoder->dst_nb_channels,
                                                     decoder->max_dst_nb_samples, decoder->dec_ctx->sample_fmt, 0);
            if (ret < 0) {
                LOGE("open decoder failed");
                return ret;
            }
            break;
        }
    }
    if (decoder->stream_index >= 0 && decoder->dec_ctx) {
        if (decoder->dst_ch_layout != decoder->dec_ctx->channel_layout
        || decoder->dst_sample_rate != decoder->dec_ctx->sample_rate) {
            decoder->swr_ctx = swr_alloc();
            if (!decoder->swr_ctx) {
                LOGE("alloc swr context failed");
                return -1;
            }
            av_opt_set_int(decoder->swr_ctx, "in_channel_layout", decoder->dec_ctx->channel_layout, 0);
            av_opt_set_int(decoder->swr_ctx, "in_sample_rate",    decoder->dec_ctx->channel_layout, 0);
            av_opt_set_sample_fmt(decoder->swr_ctx, "in_sample_fmt", decoder->dec_ctx->sample_fmt, 0);
            av_opt_set_int(decoder->swr_ctx, "out_channel_layout", decoder->dst_ch_layout, 0);
            av_opt_set_int(decoder->swr_ctx, "out_sample_rate",   decoder->dst_sample_rate, 0);
            av_opt_set_sample_fmt(decoder->swr_ctx, "out_sample_fmt", decoder->dec_ctx->sample_fmt, 0);
            if ((ret = swr_init(decoder->swr_ctx)) < 0) {
                LOGE("Failed to initialize the resampling context\n");
                return ret;
            }
        }
    }
    return 0;
}

int ff_decoder_decode(Decoder *decoder, uint8_t **buffer, int *size, int64_t *pts)
{
    int ret = 0;
    AVPacket pkt;
    AVFrame *frame;
    frame = av_frame_alloc();
    if (!frame) {
        return AVERROR(ENOMEM);
    }
    if (decoder->ifmt_ctx && !decoder->read_eof) {
        av_init_packet(&pkt);
        while (1) {
            ret = av_read_frame(decoder->ifmt_ctx, &pkt);
            if (ret < 0) {
                LOGE("read eof");
                decoder->read_eof = 1;
                break;
            }
            if (decoder->stream_index == pkt.stream_index) {
                ret = avcodec_send_frame(decoder->dec_ctx, &pkt);
                if (ret < 0) {
                    LOGE("send packet Decoder failed %s", av_err2str(ret));
                }
                ret = avcodec_receive_frame(decoder->dec_ctx, frame);
                if (ret == AVERROR(EAGAIN)) {
                    continue; // 再读一个packet 送给解码器
                } else if (ret < 0) {
                    LOGE("Decoder audio failed %s", av_err2str(ret));
                    break;
                }
                int src_sample_rate = decoder->dec_ctx->sample_rate;
                /* compute destination number of samples */
                int dst_nb_samples = av_rescale_rnd(swr_get_delay(decoder->swr_ctx, src_sample_rate) +
                                                frame->nb_samples, decoder->dst_sample_rate, src_sample_rate, AV_ROUND_UP);
                if (dst_nb_samples > decoder->max_dst_nb_samples) {
                    av_freep(&decoder->dst_data[0]);
                    ret = av_samples_alloc(decoder->dst_data, &decoder->dst_linesize, decoder->dst_nb_channels,
                                           dst_nb_samples, decoder->dec_ctx->sample_fmt, 1);
                    if (ret < 0)
                        break;
                    decoder->max_dst_nb_samples = dst_nb_samples;
                }
                // 进行重采样
                if (decoder->swr_ctx) {
                    swr_convert(decoder->swr_ctx, decoder->dst_data, dst_nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
                } else {

                }

            }
            av_packet_unref(&pkt);
        }
    }
    if (!decoder->decode_eof) {

    }
}

int ff_decoder_release(Decoder *decoder);
