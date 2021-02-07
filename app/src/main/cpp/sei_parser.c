//
// Created by chenhao on 1/22/21.
//

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include "log.h"


#define get_bits(v, start, bits) (v >> start) & ((1 << bits)-1)

int sei_parser(const char *url)
{
    int ret, i;

    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *codec_ctx = NULL;
    AVCodec *codec;
    AVPacket pkt;
    LOGI("to parser url %s", url);
    ret = avformat_open_input(&fmt_ctx,  url, NULL, NULL);
    if (!fmt_ctx) {
        LOGI("open input failed, reason is %s", av_err2str(ret));
        return -1;
    }
    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret != 0) {
        avformat_close_input(fmt_ctx);
        return -1;
    }
    int video_index = -1;
    for (i = 0; i < fmt_ctx->nb_streams; i++) {
        AVStream *stream = fmt_ctx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
            break;
        }
    }
    if (video_index < 0) {
        return -1;
    }
    AVStream  *stream = fmt_ctx->streams[video_index];
    AVCodecParameters  *parameters = stream->codecpar;
    codec = avcodec_find_decoder(parameters->codec_id);
    codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, parameters);

    ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret != 0) {
        return -1;
    }
    int packet_count = 0;
    while((ret = av_read_frame(fmt_ctx, &pkt)) >= 0)  {
        if (pkt.stream_index == video_index) {
            uint8_t *buffer = pkt.data;
            int buffer_size = pkt.size;
            uint8_t payload_type = 0;
            uint8_t payload_size = 0;
            if (buffer_size < 6) {
                continue;
            }
            int nalu_type = buffer[4] & 0x1f;
            if (buffer_size > 6 && (nalu_type == 6) && (buffer[5] == 0xf3)) { // sei
                payload_type = buffer[5];
                payload_size = buffer[6];
                LOGI("parser 到自定义数据 %d %d", payload_type, payload_size);
                uint8_t *buffer_str;
                int i = 0;
                buffer_str = malloc(payload_size);
                memset(buffer_str, 0, payload_size);
                memcpy(buffer_str, buffer + 7, payload_size);

                int64_t timestamp;
                int timezone;
                int result = 0;
                // 'ts:1598500866842;tz:8'
//                LOGI("parser data {%s} buffer[0] %x, buffer[1] %x", buffer_str, buffer_str[0], buffer_str[1]);
                result = sscanf(buffer_str, "ts:%lld;tz:%d", &timestamp, &timezone);
                __android_log_print(ANDROID_LOG_INFO, "chh", "parser sei %lld %d", timestamp,
                                    timezone);
            }
        }
        av_packet_unref(&pkt);
        if (packet_count >= 1000) {
            break;
        }
    }

    if (codec_ctx) {
        avcodec_close(codec_ctx);
        avcodec_free_context(codec_ctx);
    }
    avformat_close_input(fmt_ctx);
    return 0;
}
