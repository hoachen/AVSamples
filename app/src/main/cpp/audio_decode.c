//
// Created by chenhao on 11/21/20.
//


#include "audio_decode.h"
#include "log.h"

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"

int decode_audio(const char *src, const char *dest)
{
    int ret, i, ch;
    int audio_index = -1;
    FILE *file;
    AVFormatContext  *input_format = NULL;
    AVCodecContext  *codec_ctx = NULL;
    AVCodec  *codec = NULL;
    file = fopen(dest, "wb+");
    LOGI("start decoder %s to %s", src, dest);
    if (!file) {
        LOGI("open output filed failed %s", dest);
        return -1;
    }
    ret = avformat_open_input(&input_format, src, NULL, NULL);
    if (!input_format) {
        LOGI("avformat_open_input failed %s", av_err2str(ret));
        goto _end;
    }
    ret = avformat_find_stream_info(input_format, NULL);
    if (ret != 0) {
        LOGE("find stream info failed");
        goto _end;
    }
    LOGI("find stream info success");
//    av_dump_format(input_format, 0, src, 1);
    for (i = 0; i < input_format->nb_streams; i++) {
        AVStream *stream = input_format->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_index = i;
            break;
        }
    }
    if (audio_index < 0) {
        LOGE("input file no audio track");
        ret = -1;
        goto _end;
    }
    AVStream *audio_stream = input_format->streams[audio_index];
    codec = avcodec_find_decoder(audio_stream->codecpar->codec_id);
    if (!codec) {
        LOGE("No Found Decoder");
        ret = -1;
        goto _end;
    }
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        LOGE("can't alloc codec context");
        ret = -1;
        goto _end;
    }
    ret = avcodec_parameters_to_context(codec_ctx, audio_stream->codecpar);
    if (ret != 0) {
        LOGE("can't avcodec_parameters_to_context");
        ret = -1;
        goto _end;
    }
    ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret != 0) {
        LOGE("can't avcodec_parameters_to_context");
        ret = -1;
        goto _end;
    }
    AVPacket pkt;
    AVFrame  *frame;
    int got_frame;
    int data_size;
    frame = av_frame_alloc();
    LOGI("Start decoder");
    while ((ret = av_read_frame(input_format, &pkt)) >= 0) {
        LOGI("read pkt pts:%ld", pkt.pts);
        if (pkt.stream_index != audio_index) {
            av_packet_unref(&pkt);
            continue;
        }

        ret = avcodec_decode_audio4(codec_ctx, frame, &got_frame, &pkt);

        if (ret < 0) {
            av_packet_unref(&pkt);
            break;
        }
        if (got_frame) {
            data_size = av_get_bytes_per_sample(codec_ctx->sample_fmt);
            for (i = 0; i < frame->nb_samples; i++)
                for (ch = 0; ch < codec_ctx->channels; ch++)
                    fwrite(frame->data[ch] + data_size * i, 1, data_size, file);
        }
        av_packet_unref(&pkt);
    }
    ret = 0;
    av_packet_unref(&pkt);

_end:
    if (!frame)
        av_frame_free(&frame);
    if (codec_ctx) {
        avcodec_close(codec_ctx);
        avcodec_free_context(&codec_ctx);
    }
    if (input_format)
        avformat_close_input(&input_format);
    if (file)
        fclose(file);
    return ret;
}