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
                enc_ctx->width = dec_ctx->width;
                enc_ctx->height = dec_ctx->height;
                enc_ctx->pix_fmt = dec_ctx->pix_fmt;
                enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
                enc_ctx->time_base = av_inv_q(dec_ctx->framerate);

//                enc_ctx->width = trans->video_width;
//                enc_ctx->height = trans->video_height;
//                enc_ctx->pix_fmt = dec_ctx->pix_fmt;
//                enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
//                enc_ctx->time_base = av_inv_q(dec_ctx->framerate);


            } else {
                encoder = avcodec_find_decoder(AV_CODEC_ID_AAC);
                if (!encoder) {
                    LOGE("No found encoder for h264");
                    return AVERROR_INVALIDDATA;
                }
                enc_ctx = avcodec_alloc_context3(encoder);
                if (!enc_ctx) {
                    LOGE("Failed to allocate the encoder context\n");
                    return AVERROR(ENOMEM);
                }

                enc_ctx->sample_fmt = dec_ctx->sample_fmt;
                enc_ctx->sample_rate = dec_ctx->sample_rate;
                enc_ctx->channel_layout = dec_ctx->channel_layout;
                enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
                enc_ctx->time_base =  (AVRational){1, dec_ctx->sample_rate};

//                enc_ctx->sample_fmt = trans->dst_sample_fmt;
//                enc_ctx->sample_rate = trans->dst_sample_rate;
//                enc_ctx->channel_layout = trans->dst_channel_layout;
//                enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
//                enc_ctx->time_base =  (AVRational){1, enc_ctx->sample_rate};
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

//
//uint64_t  src_channel_layout = codec_ctx->channel_layout;
//int src_sample_rate = codec_ctx->sample_rate;
//enum AVSampleFormat src_sample_fmt = codec_ctx->sample_fmt;
//if (src_channel_layout != transcoder->dst_channel_layout ||
//src_sample_rate != transcoder->dst_sample_rate ||
//src_sample_fmt != transcoder->dst_sample_fmt) {
//// 初始化音频重采样器
//transcoder->swr_ctx = swr_alloc();
//if (!transcoder->swr_ctx) {
//LOGE("Could not allocate resampler context\n");
//ret = AVERROR(ENOMEM);
//return ret;
//}
///* set options */
//av_opt_set_int(transcoder->swr_ctx, "in_channel_layout",  src_channel_layout, 0);
//av_opt_set_int(transcoder->swr_ctx, "in_sample_rate",       src_sample_rate, 0);
//av_opt_set_sample_fmt(transcoder->swr_ctx, "in_sample_fmt", src_sample_fmt, 0);
//av_opt_set_int(transcoder->swr_ctx, "out_channel_layout",    transcoder->dst_channel_layout, 0);
//av_opt_set_int(transcoder->swr_ctx, "out_sample_rate",       transcoder->dst_sample_rate, 0);
//av_opt_set_sample_fmt(transcoder->swr_ctx, "out_sample_fmt", transcoder->dst_sample_fmt, 0);
///* initialize the resampling context */
//if ((ret = swr_init(transcoder->swr_ctx)) < 0) {
//fprintf(stderr, "Failed to initialize the resampling context\n");
//return ret;
//}
//}

int decode_frame(Transcoder *trans, int stream_index, AVFrame *frame, int *got_frame) {
    int ret = 0;
    AVCodecContext *dec_ctx = trans->stream_ctx[stream_index].dec_ctx;
    ret = avcodec_receive_frame(dec_ctx, frame);
    if (ret >= 0) {
        *got_frame = 1;
    } else if (ret == AVERROR(EAGAIN)) {
        return 0;
    }
    return 0;
}

int encode_write_frame(Transcoder *trans, AVFrame *frame, int stream_index) {
    int ret = 0;
    AVCodecContext *enc_ctx;
    AVPacket enc_pkt;

    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);

    enc_ctx = trans->stream_ctx[stream_index].enc_ctx;
    ret = avcodec_send_frame(enc_ctx, frame);
    av_frame_free(&frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        ret = 0;
    } else if (ret < 0) {
        LOGI("send frame to encoder failed %s", av_err2str(ret));
        return ret;
    } else {
        ret = avcodec_receive_packet(enc_ctx, &enc_pkt);
        if (ret == 0) {
            enc_pkt.stream_index = stream_index;
            av_packet_rescale_ts(&enc_pkt,
                                 enc_ctx->time_base,
                                 trans->ofmt_ctx->streams[stream_index]->time_base);
            LOGI("Muxing a frame stream_index %d, pts %ld\n", enc_pkt.stream_index, enc_pkt.pts);
            /* mux encoded frame */
            ret = av_interleaved_write_frame(trans->ofmt_ctx, &enc_pkt);
        } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            ret = 0;
        }
    }
    av_packet_unref(&enc_pkt);
    return ret;
}

int transcode_start(Transcoder *transcoder)
{
    int ret = 0;
    AVPacket pkt = { .data = NULL, .size = 0 };
    AVFrame *frame = NULL;
    unsigned int stream_index;
    unsigned int i;
    int got_frame = 0;
    if ((ret = open_input_file(transcoder)) < 0) {
        return ret;
    }
    if ((ret = open_output_file(transcoder)) < 0) {
        return ret;
    }

    while (1) {
        if ((ret = av_read_frame(transcoder->ifmt_ctx, &pkt)) < 0) {
            break;
        }
        stream_index = pkt.stream_index;
        frame = av_frame_alloc();
        if (!frame) {
            ret = AVERROR(ENOMEM);
            break;
        }
        AVCodecContext *dec_ctx = transcoder->stream_ctx[stream_index].dec_ctx;
        av_packet_rescale_ts(&pkt,
                             transcoder->ifmt_ctx->streams[stream_index]->time_base,
                             dec_ctx->time_base);
        ret = avcodec_send_packet(dec_ctx, &pkt);
        LOGI("send a pkt to decoder #stream_index %d, pks %ld, dts %ld, ret=%d", pkt.stream_index, pkt.pts, pkt.dts, ret);
        if (ret == AVERROR(EAGAIN)) {
            av_packet_unref(&pkt);
            continue;
        }
        ret = decode_frame(transcoder, stream_index, frame, &got_frame);
        if (ret < 0) {
            LOGE("decoder a frame failed stream index #%u", stream_index);
            break;
        }
        if (got_frame) {
            ret = encode_write_frame(transcoder, frame, stream_index);
//            if (ret < 0) {
//                break;
//            }
        }
        av_packet_unref(&pkt);
    }
    av_write_trailer(transcoder->ofmt_ctx);
    return ret;
}

int transcode_stop(Transcoder *transcoder);

void transcode_destroy(Transcoder *transcoder);