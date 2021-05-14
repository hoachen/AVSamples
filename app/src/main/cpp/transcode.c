//
// Created by chenhao on 2021/5/13.
//

#include "transcode.h"

Transcoder *create_transcoder()
{
    Transcoder *transcoder = malloc(sizeof(Transcoder));
    if (transcoder) {
        memset(transcoder, 0, sizeof(Transcoder));
        transcoder->audio_stream_idx = -1;
        transcoder->video_stream_idx = -1;
        transcoder->ifmt_ctx = NULL;
        transcoder->ofmt_ctx = NULL;
        transcoder->stream_ctx = NULL;
        transcoder->sws_ctx = NULL;
        transcoder->swr_ctx = NULL;
        transcoder->dst_channel_layout = AV_CH_LAYOUT_STEREO;
        transcoder->dst_sample_rate = 44100;
        transcoder->dst_sample_fmt = AV_SAMPLE_FMT_S16;
        transcoder->output_video_pts = 0;
        transcoder->output_audio_pts = 0;
    }
    return transcoder;
}

int transcode_init(Transcoder *transcoder, const char *input_file, const char *output_file,
         int width, int height, int fps, int v_bitrate,
         int sample_rate, int channel, int a_bitrate)
{
    int ret = 0;
    if (transcoder) {
        strncpy(transcoder->input_file, input_file, strlen(input_file));
        strncpy(transcoder->output_file, output_file, strlen(output_file));
        transcoder->video_width = width;
        transcoder->video_height = height;
        transcoder->fps = fps;
        transcoder->video_bitrate = v_bitrate;
        transcoder->dst_sample_rate = sample_rate;
        transcoder->dst_channel_layout = (channel == 2) ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;
        transcoder->audio_bitrate = a_bitrate;
    }
    return ret;
}

int open_input_file(Transcoder *transcoder)
{
    AVFormatContext *ifmt_ctx = NULL;
    int ret = 0;
    unsigned i = 0;
    if ((ret = avformat_open_input(&ifmt_ctx, transcoder->input_file, NULL, NULL))  < 0) {
        LOGE("open input file failed err: %s !!!" , av_err2str(ret));
        return ret;
    }
    if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
        LOGE("find stream info failed err : %s" , av_err2str(ret));
        return ret;
    }
    transcoder->ifmt_ctx = ifmt_ctx;
    transcoder->stream_ctx = av_mallocz_array(ifmt_ctx->nb_streams, sizeof(StreamContext));
    if (!transcoder->stream_ctx) {
        return AVERROR(ENOMEM);
    }
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *stream = ifmt_ctx->streams[i];
        AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
        AVCodecContext *codec_ctx;
        if (!dec) {
            LOGE("Failed to find decoder for stream #%u", i);
            return AVERROR_DECODER_NOT_FOUND;
        }
        codec_ctx = avcodec_alloc_context3(dec);
        if (!codec_ctx) {
            LOGE("Failed to allocate the decoder context for stream #%u", i);
            return AVERROR(ENOMEM);
        }
        ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
        if (ret != 0) {
            LOGE("Failed to copy decoder parameters to input decoder context "
                                       "for stream #%u", i);
            return ret;
        }
        if (codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO || codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
                codec_ctx->framerate = av_guess_frame_rate(ifmt_ctx, stream, NULL);
                transcoder->video_stream_idx = i;
            } else {
                transcoder->audio_stream_idx = i;
            }
            ret = avcodec_open2(codec_ctx, dec, NULL);
            if (ret < 0) {
                LOGE("Failed to open decoder for stream #%u", i);
                return ret;
            }
        }
        transcoder->stream_ctx[i].dec_ctx = codec_ctx;
        transcoder->stream_ctx[i].dec_frame = av_frame_alloc();
        if (!transcoder->stream_ctx[i].dec_frame) {
            return AVERROR(ENOMEM);
        }
    }
    return 0;
}


int open_output_file(Transcoder *trans)
{
    int ret = 0;
    unsigned int i ;
    AVFormatContext *ofmt_ctx = NULL;
    AVStream *in_stream = NULL;
    AVStream *out_stream = NULL;
    AVCodec *encoder = NULL;
    AVCodecContext  *enc_ctx = NULL;
    ret = avformat_alloc_output_context2(&ofmt_ctx, NULL, "mp4", trans->output_file);
    if (!ofmt_ctx) {
        LOGE("open output file failed err: %s", av_err2str(ret));
        return ret;
    }
    trans->ofmt_ctx = ofmt_ctx;
    for (i = 0; i < trans->ifmt_ctx->nb_streams; i++) {
        out_stream = avformat_new_stream(ofmt_ctx, NULL);
        if (!out_stream) {
            LOGE("create out stream failed for stream #%u", i);
            return -1;
        }
        in_stream = trans->ifmt_ctx->streams[i];
        AVCodecContext* dec_ctx = in_stream->codec;
        if (dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO || dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
                encoder = avcodec_find_decoder(AV_CODEC_ID_H264);
                if (!encoder) {
                    LOGE("No found encoder for h264");
                    return AVERROR_INVALIDDATA;
                }
                enc_ctx = avcodec_alloc_context3(encoder);
                if (!enc_ctx) {
                    LOGE("Failed to allocate the encoder context\n");
                    return AVERROR(ENOMEM);
                }
                enc_ctx->width = trans->video_width;
                enc_ctx->height = trans->video_height;
                enc_ctx->bit_rate = trans->video_bitrate;
                enc_ctx->pix_fmt = AV_PIX_FMT_NV12;
//                enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
                enc_ctx->framerate = (AVRational){trans->fps,1};
                enc_ctx->time_base = av_inv_q(enc_ctx->framerate);
                out_stream->time_base = (AVRational){1, enc_ctx->framerate.num};
                enc_ctx->time_base = out_stream->time_base;
            } else {
                encoder = avcodec_find_decoder(AV_CODEC_ID_AAC);
                if (!encoder) {
                    LOGE("No found encoder for aac");
                    return AVERROR_INVALIDDATA;
                }
                enc_ctx = avcodec_alloc_context3(encoder);
                if (!enc_ctx) {
                    LOGE("Failed to allocate the encoder context\n");
                    return AVERROR(ENOMEM);
                }
                enc_ctx->sample_rate = trans->dst_sample_rate;
                enc_ctx->channel_layout = trans->dst_channel_layout;
                enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
                /* take first format from list of supported formats */
                enc_ctx->sample_fmt = trans->dst_sample_fmt;
                enc_ctx->time_base = (AVRational){1, enc_ctx->sample_rate};
            }
            if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            ret = avcodec_open2(enc_ctx, encoder, NULL);
            if (ret < 0) {
                LOGE("Cannot open video encoder for stream #%u\n", i);
                return ret;
            }
            ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
            if (ret < 0) {
                LOGI("Failed to copy encoder parameters to output stream #%u\n", i);
                return ret;
            }
            out_stream->time_base = enc_ctx->time_base;
            trans->stream_ctx[i].enc_ctx = enc_ctx;
        } else if (dec_ctx->codec_type == AVMEDIA_TYPE_UNKNOWN) {
            LOGE("Elementary stream #%d is of unknown type, cannot proceed\n", i);
            return AVERROR_INVALIDDATA;
        } else {
            /* if this stream must be remuxed */
            ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
            if (ret < 0) {
                LOGE("Copying parameters for stream #%u failed\n", i);
                return ret;
            }
            out_stream->time_base = in_stream->time_base;
        }
    }
    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, trans->output_file, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("Could not open output file '%s'", trans->output_file);
            return ret;
        }
    }
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        LOGE("Error occurred when opening output file\n");
        return ret;
    }
    return 0;

}


AVFrame* new_video_frame(enum AVPixelFormat pixfmt,int width,int height)
{
    AVFrame *video_frame = av_frame_alloc();
    video_frame->format = pixfmt;
    video_frame->width = width;
    video_frame->height = height;
    int ret = 0;
    if ((ret = av_frame_get_buffer(video_frame, 0)) < 0) {
        LOGD("video get frame buffer fail %d",ret);
        return NULL;
    }
    if ((ret =  av_frame_make_writable(video_frame)) < 0) {
        LOGD("video av_frame_make_writable fail %d",ret);
        return NULL;
    }
    return video_frame;
}

AVFrame* new_audio_frame(enum AVSampleFormat sample_fmt, int64_t ch_layout,int sample_rate, int nb_samples)
{
    AVFrame * audio_en_frame = av_frame_alloc();
    // 根据采样格式，采样率，声道类型以及采样数分配一个AVFrame
    audio_en_frame->format = sample_fmt;
    audio_en_frame->sample_rate = sample_rate;
    audio_en_frame->channel_layout = ch_layout;
    audio_en_frame->nb_samples = nb_samples;
    int ret = 0;
    if ((ret = av_frame_get_buffer(audio_en_frame, 0)) < 0) {
        LOGD("audio get frame buffer fail %d",ret);
        return NULL;
    }
    if ((ret =  av_frame_make_writable(audio_en_frame)) < 0) {
        LOGD("audio av_frame_make_writable fail %d",ret);
        return NULL;
    }
    return audio_en_frame;
}

int convert_video_frame(struct SwsContext *sws_ctx, AVFrame *src_frame, AVFrame *dst_frame) {
    int ret = 0;
    if (sws_ctx) {
        ret = sws_scale(sws_ctx, src_frame->data, src_frame->linesize,
                        0, src_frame->height, dst_frame->data, dst_frame->linesize);
    } else {
        av_frame_copy(dst_frame, src_frame);
    }
    return ret;
}

int convert_audio_frame(struct SwrContext *swr_ctx, AVCodecContext * enc_ctx, AVFrame *src_frame, AVFrame *dst_frame) {
    int pts_num = 0;
    int ret = 0;
    if (swr_ctx) {
        int dst_nb_samples = (int)av_rescale_rnd(swr_get_delay(swr_ctx, src_frame->sample_rate) + dst_frame->nb_samples, enc_ctx->sample_rate, src_frame->sample_rate, AV_ROUND_UP);
        if (dst_nb_samples != dst_frame->nb_samples) {
            av_frame_free(&dst_frame);
            dst_frame = new_audio_frame(enc_ctx->sample_fmt, enc_ctx->channel_layout, enc_ctx->sample_rate, dst_nb_samples);
            if (dst_frame == NULL) {
                LOGD("can not create audio frame2 ");
                return AVERROR(ENOMEM);
            }
        }
        /** 遇到问题：当音频编码方式不一致时转码后无声音
         *  分析原因：因为每个编码方式对应的AVFrame中的nb_samples不一样，所以再进行编码前要进行AVFrame的转换
         *  解决方案：进行编码前先转换
         */
        // 进行转换
        ret = swr_convert(swr_ctx, dst_frame->data, dst_nb_samples, (const uint8_t**)src_frame->data, src_frame->nb_samples);
        if (ret < 0) {
            LOGD("swr_convert() fail %d",ret);
            return ret;
        }
        pts_num = ret;
    } else {
        av_frame_copy(dst_frame, src_frame);
        pts_num = dst_frame->nb_samples;
    }
    return ret;

}

int encode_write_frame(Transcoder *trans, unsigned int stream_index, int flush) {
    StreamContext *stream = &trans->stream_ctx[stream_index];
    int ret;
    if (stream->enc_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        if (!stream->enc_frame) {
            stream->enc_frame = new_video_frame(stream->enc_ctx->pix_fmt,
                                                stream->enc_ctx->width,
                                                stream->enc_ctx->height);
            if (!stream->enc_frame) {
                return  AVERROR(ENOMEM);
            }
            convert_video_frame(trans->sws_ctx, stream->dec_frame, stream->enc_frame);
        }
        stream->enc_frame->pts = trans->output_video_pts++;
    } else {
        stream->enc_frame = new_audio_frame(stream->enc_ctx->sample_fmt,
                                            stream->enc_ctx->channel_layout,
                                            stream->enc_ctx->sample_rate,
                                            stream->enc_ctx->frame_size);
        if (!stream->enc_frame) {
            if (!stream->enc_frame) {
                return AVERROR(ENOMEM);
            }
            convert_audio_frame(trans->swr_ctx, stream->enc_ctx, stream->dec_frame, stream->enc_frame);
        }
        stream->enc_frame->pts = av_rescale_q(trans->output_audio_pts, (AVRational){1,stream->enc_frame->sample_rate}, stream->dec_ctx->time_base);
        trans->output_audio_pts += stream->enc_frame->nb_samples;
    }
    LOGI("Encoding a frame stream index #%u\n", stream_index);
    ret = avcodec_send_frame(stream->enc_ctx, stream->enc_frame);
    if (ret < 0) {
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            ret = 0;
        }
        LOGI("encode failed %s\n", av_err2str(ret));
        return ret;
    }
    AVPacket enc_pkt;
    av_init_packet(&enc_pkt);
    while (ret >= 0) {
        ret = avcodec_receive_packet(stream->enc_ctx, &enc_pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return 0;
        /* prepare packet for muxing */
        enc_pkt.stream_index = stream_index;
        av_packet_rescale_ts(&enc_pkt,
                             stream->enc_ctx->time_base,
                             trans->ofmt_ctx->streams[stream_index]->time_base);

        LOGD("Muxing frame\n");
        /* mux encoded frame */
        ret = av_interleaved_write_frame(trans->ofmt_ctx, &enc_pkt);
        av_packet_unref(&enc_pkt);
    }

    return ret;
}

int transcode_start(Transcoder *transcoder)
{
    int ret = 0;
    AVPacket *pkt = NULL;
    unsigned int stream_index;
    unsigned int i;
    if ((ret = open_input_file(transcoder)) < 0) {
        return ret;
    }
    if ((ret = open_output_file(transcoder)) < 0) {
        return ret;
    }
    if (!(pkt = av_packet_alloc())) {
        return AVERROR(ENOMEM);
    }
    // 初始化视频重采样
    if (transcoder->video_stream_idx >= 0) {
        StreamContext *stream = &transcoder->stream_ctx[transcoder->video_stream_idx];
        if (stream->enc_ctx->pix_fmt != stream->dec_ctx->pix_fmt ||
             stream->enc_ctx->width != stream->dec_ctx->width ||
             stream->enc_ctx->height != stream->dec_ctx->height) {
            transcoder->sws_ctx = sws_getContext(stream->dec_ctx->width, stream->dec_ctx->height, (enum AVPixelFormat)stream->dec_ctx->pix_fmt,
                                                 stream->enc_ctx->width, stream->enc_ctx->height, stream->enc_ctx->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
            if (!transcoder->sws_ctx) {
                LOGE("init sws_context failed");
                return -1;
            }
        }
    }
    // 初始化
    if (transcoder->audio_stream_idx >= 0) {
        StreamContext *stream = &transcoder->stream_ctx[transcoder->audio_stream_idx];
        if (stream->enc_ctx->sample_rate != stream->dec_ctx->sample_rate ||
            stream->enc_ctx->channel_layout != stream->dec_ctx->channel_layout ||
            stream->enc_ctx->sample_fmt != stream->dec_ctx->sample_fmt ||
            stream->enc_ctx->frame_size != stream->dec_ctx->frame_size) {
            transcoder->swr_ctx = swr_alloc_set_opts(NULL, stream->enc_ctx->channel_layout,
                                                     stream->enc_ctx->sample_fmt, stream->enc_ctx->sample_rate,
                                                     stream->dec_ctx->channel_layout,
                                                     (enum AVSampleFormat)stream->dec_ctx->sample_fmt,
                                                     stream->dec_ctx->sample_rate, 0, NULL);
            if ((ret = swr_init(transcoder->swr_ctx)) < 0) {
                LOGD("swr_alloc_set_opts() fail %d",ret);
                return -1;
            }
        }
    }
    while (1) {
        if ((ret = av_read_frame(transcoder->ifmt_ctx, pkt)) < 0) {
            break;
        }
        stream_index = pkt->stream_index;
        StreamContext *stream = &transcoder->stream_ctx[stream_index];
        av_packet_rescale_ts(pkt,
                             transcoder->ifmt_ctx->streams[stream_index]->time_base,
                             stream->dec_ctx->time_base);
        ret = avcodec_send_packet(stream->dec_ctx, pkt);
        LOGI("send a pkt to decoder #stream_index %d, pks %ld, dts %ld, ret=%d", pkt->stream_index, pkt->pts, pkt->dts, ret);
        if (ret < 0) {
            LOGE("Decoding failed %s \n", av_err2str(ret));
            break;
        }
        int exit = 0;
        while (ret >= 0) {
            ret = avcodec_receive_frame(stream->dec_ctx, stream->dec_frame);
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
                break;
            } else if (ret < 0) {
                exit = 1;
                break;
            }
            stream->dec_frame->pts = stream->dec_frame->best_effort_timestamp;
            ret = encode_write_frame(transcoder, stream_index, 0);
            if (ret < 0)
                exit = 1;
        }
        if (exit) {
            break;
        }
        av_packet_unref(pkt);
    }
    av_write_trailer(transcoder->ofmt_ctx);
    return ret;
}

int transcode_stop(Transcoder *transcoder);

void transcode_destroy(Transcoder *transcoder);