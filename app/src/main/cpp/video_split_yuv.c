//
// Created by chenhao on 12/30/20.
//

#include "video_split_yuv.h"
#include "log.h"

int write_yuv420_to_file(FILE *file, int width, int height, AVFrame *frame)
{
    if(!file || !frame) {
        return -1;
    }
    LOGI("Write a frame to yuv file %dx%d frame %dx%d", width, height, frame->width, frame->height);
//    fwrite(frame->data[0],1, width * height,   file);     //Y
//    fwrite(frame->data[1],1, width * height / 4, file);  //U
//    fwrite(frame->data[2],1, width * height / 4, file);  //V
    int y_size = width * height;
    fwrite(frame->data[0], 1, y_size, file);
    fwrite(frame->data[1], 1, y_size / 4, file);
    fwrite(frame->data[2], 1, y_size / 4, file);
//
//    // 写Y分量
//    for (size_t i = 0; i < frame->height; i++)
//    {
//        fwrite(frame->data[0] + frame->linesize[0] * i, 1, frame->width, file);
//    }
//
//    // 写U分量
//    for (size_t i = 0; i < frame->height / 2; i++)
//    {
//        fwrite(frame->data[1] + frame->linesize[1] * i, 1, frame->width / 2, file);
//    }
//
//    // 写V分量
//    for (size_t i = 0; i < frame->height / 2; i++)
//    {
//        fwrite(frame->data[2] + frame->linesize[2] * i, 1, frame->width / 2, file);
//    }


    LOGI("Write a frame to yuv file 1");
    return 0;
}

int decoder_video(AVCodecContext *decode_ctx, AVPacket *pkt, AVFrame *frame)
{
    int ret;
    ret = avcodec_send_packet(decode_ctx, pkt);
    LOGI("avcodec_send_packet %d", ret);
    if (ret < 0) {
        return -1;
    }
    ret = avcodec_receive_frame(decode_ctx, frame);
    LOGI("avcodec_receive_frame %d", ret);
    if ((ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)) {
        return 0;
    }
    if (ret < 0) {
        return -1;
    }
    LOGI("avcodec_receive_frame 1 ")
    return 1;
}

int split_yuv(const char *input_file, const char *output_dir)
{
    LOGI("video split to yuv start");
    int video_index = -1;
    int ret = 0, i = 0;
    int index = 0;
    FILE *file = NULL;
    AVFormatContext  *ic = NULL;
    AVCodecContext  *decode_ctx;
    AVCodec         *decode;
    AVPacket pkt;
    AVFrame *frame = NULL;
    char *filename[1024] = {'\0'};
    sprintf(filename, "%s/temp.yuv", output_dir);

    file = fopen(filename, "wb");
    if (!file) {
        LOGE("open file %s failed..", filename);
        return -1;
    }
    LOGI("start split video %s to %s", input_file, filename);
    ret = avformat_open_input(&ic, input_file, NULL, NULL);
    if (ret != 0) {
        LOGI("open input format failed %s", av_err2str(ret));
        return AVERROR(ret);
    }
    ret = avformat_find_stream_info(ic, NULL);
    if (ret != 0) {
        avformat_close_input(&ic);
        return AVERROR(ret);
    }
    for (i = 0; i < ic->nb_streams; i++) {
        AVStream *stream = ic->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
            break;
        }
    }
    if (video_index < 0) {
        avformat_close_input(&ic);
        LOGI("not found video stream");
        return -1;
    }
    AVStream  *video_stream = ic->streams[video_index];
    decode = avcodec_find_decoder(video_stream->codecpar->codec_id);
    if (decode == NULL) {
        avformat_close_input(&ic);
        LOGI("not found video decoder");
        return -1;
    }
    decode_ctx = avcodec_alloc_context3(decode);
    if (decode_ctx == NULL) {
        avformat_close_input(&ic);
        LOGI("alloc decoder context failed");
        return -1;
    }
    avcodec_parameters_to_context(decode_ctx, video_stream->codecpar);
    ret = avcodec_open2(decode_ctx, decode, NULL);
    if (ret != 0) {
        avcodec_free_context(&decode_ctx);
        avformat_close_input(&ic);
        return -1;
    }

    int frame_count = 0;
    int got_frame = 0;
    frame = av_frame_alloc();
    while ((ret = av_read_frame(ic, &pkt)) >= 0) {
        if (pkt.stream_index == video_index) {
            LOGI("to decoder video");
            got_frame = decoder_video(decode_ctx, &pkt, frame);
            if (got_frame < 0) {
                LOGI("decoder failed");
                break;
            } else if (got_frame == 0) {
                av_packet_unref(&pkt);
                continue;
            }
            LOGI("to decoder video %d", got_frame);
            if (got_frame == 1) {
                write_yuv420_to_file(file, decode_ctx->width, decode_ctx->height, &frame);
                frame_count++;
            }
        }
        av_packet_unref(&pkt);
        if (frame_count >= 250)
            break;
    }
    if (frame) {
        av_frame_free(&frame);
    }
    if (decode_ctx) {
        avcodec_close(decode_ctx);
        avcodec_free_context(decode_ctx);
    }

    if (ic != NULL)
        avformat_close_input(&ic);
    if (file)
        fclose(file);
    LOGI("video split to yuv done");
    return index;
}