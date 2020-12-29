//
// Created by chenhao on 12/29/20.
//

#include "video_split.h"
#include "log.h"

static int start_write_item(AVFormatContext **oc, AVStream *in_stream, AVStream **out_stream, const char *output_dir, int index)
{
    int ret = 0;
    char filename[1024] = {'\0'};
    sprintf(filename, "%s/temp_%d.mp4", output_dir, index);
    LOGI("start write to temp file %s", filename);
    ret = avformat_alloc_output_context2(oc, NULL, "mp4", filename);
    if (ret) {
        return AVERROR(ret);
    }
    *out_stream = avformat_new_stream(*oc, NULL);
    if (*out_stream == NULL) {
        LOGE("avformat_new_stream failed");
        return -1;
    }
    ret = avcodec_parameters_copy((*out_stream)->codecpar, in_stream->codecpar);
    if (ret < 0) {
        LOGE("avcodec_parameters_copy failed");
        return -1;
    }

    if (!((*oc)->flags & AVFMT_NOFILE)) {
        ret = avio_open(&(*oc)->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("Could not open output file '%s'", filename);
            return AVERROR(ret);
        }
    }
//    av_dump_format(oc, 0, filename, 1);
    ret = avformat_write_header(*oc, NULL);
    LOGI("avformat_write_header end ret = %d", ret);
    return ret;
}

static int write_item_packet(AVFormatContext *oc, AVStream *in_stream, AVStream *out_stream,
        AVPacket *pkt, int64_t pts, int64_t dts)
{
    if (oc == NULL) {
        LOGE("output context is NULL");
        return -1;
    }
    int ret;
//    pkt->pts = pts;
//    pkt->dts = dts;
    pkt->pts = av_rescale_q_rnd(pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    pkt->dts = av_rescale_q_rnd(dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
    pkt->pos = -1;
    LOGI("write_item_packet pks %ld, dts %ld, duration %ld start", pkt->pts, pkt->dts, pkt->duration);
    ret = av_interleaved_write_frame(oc, pkt);
    LOGI("write_item_packet pks %ld, dts %ld, duration %ld end", pkt->pts, pkt->dts, pkt->duration);
    return ret;
}

static int close_write_item(AVFormatContext **oc)
{
    if (*oc != NULL) {
        LOGI("close write to temp file %s", (*oc)->filename);
        av_write_trailer(*oc);
        if (!((*oc)->flags & AVFMT_NOFILE))
            avio_closep(&(*oc)->pb);
        avformat_free_context(*oc);
    }
    return 0;
}

int split_video(const char *input_file, const char *output_dir)
{
    int video_index = -1;
    int ret = 0, i = 0;
    int index = 0;
    AVFormatContext  *ic = NULL;
    AVFormatContext  *oc = NULL;
    AVStream  *in_stream = NULL;
    AVStream  *out_stream = NULL;
    AVPacket pkt;
    LOGI("start split video %s to %s", input_file, output_dir);
    ret = avformat_open_input(&ic, input_file, NULL, NULL);
    if (ret != 0) {
        LOGI("avformat_open_input failed %s", av_err2str(ret));
        return AVERROR(ret);
    }
    ret = avformat_find_stream_info(ic, NULL);
    if (ret != 0) {
        avformat_close_input(&ic);
        return AVERROR(ret);
    }
//    av_dump_format(ic, 0, input_file, 1);
    for (i = 0; i < ic->nb_streams; i++) {
        AVStream *stream = ic->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            in_stream = stream;
            video_index = i;
            break;
        }
    }
    if (video_index < 0) {
        avformat_close_input(&ic);
        LOGI("not found video stream");
        return -1;
    }
    int64_t last_pts = 0, last_dts = 0;
    while ((ret = av_read_frame(ic, &pkt)) >= 0) {
        if (pkt.stream_index == video_index) {
            if (pkt.flags & AV_PKT_FLAG_KEY) { // 判断是I帧
                close_write_item(&oc);
                start_write_item(&oc, in_stream, &out_stream, output_dir, index++);
                last_pts = pkt.pts;
                last_dts = pkt.dts;
            }
            int64_t pts = pkt.pts - last_pts;
            int64_t dts = pkt.dts - last_dts;
            LOGI("start write packet to %d file, pts %ld, dts %ld", index, pts, dts);
            write_item_packet(oc, in_stream, out_stream, &pkt, pts, dts);
            LOGI("start write packet to %d file, pts %ld, dts %ld end", index, pts, dts);
        }
        av_packet_unref(&pkt);
    }
    LOGI("video split end");
    close_write_item(&oc);
    if (ic != NULL)
        avformat_close_input(&ic);
    LOGI("video split done");
    return 0;
}