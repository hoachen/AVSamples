//
// Created by chenhao on 12/29/20.
//

#include "rplayer.h"

#define TEMP_FILE_NAME "temp"

int segment_queue_init(SegmentQueue *q)
{
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
    q->head = NULL;
    q->tail = NULL;
    q->duration = 0;
    q->frame_count = 0;
    q->size = 0;
    q->abort_request = 0;
}

int segment_queue_abort(SegmentQueue *q)
{
    pthread_mutex_lock(&q->mutex);
    q->abort_request = 1;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

int segment_queue_put_l(SegmentQueue *q, Segment *segment)
{
    if (q->abort_request)
        return -1;
    SegmentNode *s1 = (SegmentNode *)malloc(sizeof(SegmentNode));
    if (!segment) {
        LOGE("malloc SegmentNode failed");
        return -1;
    }
    memset(s1, 0, sizeof(SegmentNode));
    s1->next = NULL;
    s1->segment = *segment;

    if (q->tail) {
        q->tail->next = s1;
    }
    q->tail = s1;
    if (!q->head) {
        q->head = q->tail;
    }
    q->duration += segment->duration;
    q->size ++;
    q->frame_count += segment->frames;
    pthread_cond_signal(&q->cond);
    return 0;
}

int segment_queue_put(SegmentQueue *q, Segment *segment)
{
    int ret = 0;
    pthread_mutex_lock(&q->mutex);
    ret = segment_queue_put_l(q, segment);
    pthread_mutex_unlock(&q->mutex);
    return ret;
}

int segment_queue_get(SegmentQueue *q, Segment *segment, int block)
{
    int ret;
    SegmentNode *s1 = NULL;
    pthread_mutex_lock(&q->mutex);
    for (;;) {
        if (q->abort_request) {
            ret = -1;
            break;
        }
        if (q->head) {
            s1 = q->head;
            q->head = s1->next;
            if (!q->head) {
                q->tail = NULL;
            }
            ret = 1;
            *segment = s1->segment;
            q->size--;
            q->duration -= segment->duration;
            q->frame_count -= segment->frames;
            // !! free();
            free(s1);
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            pthread_cond_wait(&q->cond, &q->mutex);
        }
    }
    pthread_mutex_unlock(&q->mutex);
    return ret;
}

int segment_queue_flush(SegmentQueue *q)
{
    SegmentNode *s1, *s2;
    pthread_mutex_unlock(&q->mutex);
    for (s1 = q->head; s1; s1 = s2) {
        s2 = s1->next;
        free(s1);
    }
    q->head = NULL;
    q->tail = NULL;
    q->frame_count = 0;
    q->size = 0;
    q->duration = 0;
    pthread_mutex_unlock(&q->mutex);
}

int segment_queue_destroy(SegmentQueue *q)
{
    segment_queue_flush(q);
    pthread_cond_destroy(&q->cond);
    pthread_mutex_destroy(&q->mutex);
}


int write_yuv420_to_file(FILE *file, int width, int height, AVFrame *frame)
{
    if(!file || !frame) {
        return -1;
    }
//    LOGI("Write a frame to yuv file %dx%d frame %dx%d", width, height, frame->width, frame->height);
    fwrite(frame->data[0],1, width * height,   file);     //Y
    fwrite(frame->data[1],1, width * height / 4, file);  //U
    fwrite(frame->data[2],1, width * height / 4, file);  //V
//    int y_size = width * height;
//    fwrite(frame->data[0], 1, y_size, file);
//    fwrite(frame->data[1], 1, y_size / 4, file);
//    fwrite(frame->data[2], 1, y_size / 4, file);
//    LOGI("Write a frame to yuv file 1");
    return 0;
}

int decoder_video(AVCodecContext *decode_ctx, AVPacket *pkt, AVFrame *frame)
{
    int ret;
    ret = avcodec_send_packet(decode_ctx, pkt);
    if (ret < 0) {
        return -1;
    }
    ret = avcodec_receive_frame(decode_ctx, frame);
    if ((ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)) {
        return 0;
    }
    if (ret < 0) {
        return -1;
    }
//    LOGI("avcodec_receive_frame 1 ")
    return 1;
}

int decode_to_yuv420(const char *input_file, const char *output_file)
{
//    LOGI("video split to yuv start");
    int video_index = -1;
    int ret = 0, i = 0;
    FILE *file = NULL;
    AVFormatContext  *ic = NULL;
    AVCodecContext  *decode_ctx;
    AVCodec         *decode;
    AVPacket pkt;
    AVFrame *frame = NULL;
    AVFrame *frame_yuv = NULL;
    struct SwsContext *sws_ctx = NULL;

    file = fopen(output_file, "wb");
    if (!file) {
        LOGE("open file %s failed..", output_file);
        return -1;
    }
//    LOGI("start decode video %s to %s", input_file, output_file);
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

    frame = av_frame_alloc();
    frame_yuv = av_frame_alloc();

    uint8_t *out_buffer = (uint8_t *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, decode_ctx->width, decode_ctx->height,1));
    //初始化缓冲区
    av_image_fill_arrays(frame_yuv->data, frame_yuv->linesize, out_buffer, AV_PIX_FMT_YUV420P, decode_ctx->width, decode_ctx->height, 1);
    sws_ctx = sws_getContext(decode_ctx->width, decode_ctx->height, decode_ctx->pix_fmt,
                             decode_ctx->width, decode_ctx->height, AV_PIX_FMT_YUV420P,
                             SWS_BICUBIC, NULL, NULL, NULL);
    int frame_count = 0;
    int got_frame = 0;


    while ((ret = av_read_frame(ic, &pkt)) >= 0) {
        if (pkt.stream_index == video_index) {
//            LOGI("to decoder video");
            got_frame = decoder_video(decode_ctx, &pkt, frame);
            if (got_frame < 0) {
                LOGI("decoder failed");
                break;
            } else if (got_frame == 0) {
                av_packet_unref(&pkt);
                continue;
            }
//            LOGI("to decoder video %d", got_frame);
            if (got_frame == 1) {
//                sws_scale(sws_ctx, frame->data, frame->linesize, 0, decode_ctx->height,
//                          frame_yuv->data, frame_yuv->linesize);
                write_yuv420_to_file(file, decode_ctx->width, decode_ctx->height, frame);
                frame_count++;
            }
        }
        av_packet_unref(&pkt);

    }
    avcodec_flush_buffers(decode_ctx);
    got_frame = avcodec_receive_frame(decode_ctx, frame);
    if (got_frame == 0) {
//        LOGI("avcodec_flush_buffers frame");
        frame_count++;
//        sws_scale(sws_ctx, frame->data, frame->linesize, 0, decode_ctx->height,
//                  frame_yuv->data, frame_yuv->linesize);
        write_yuv420_to_file(file, decode_ctx->width, decode_ctx->height, frame);
    }

    if (frame) {
        av_frame_free(&frame);
    }
    if (decode_ctx) {
        avcodec_close(decode_ctx);
        avcodec_free_context(&decode_ctx);
    }

    if (ic != NULL)
        avformat_close_input(&ic);
    if (file)
        fclose(file);
//    LOGI("decode video %s to %s done", input_file, output_file);
    return frame_count;
}

static int start_write_item(AVFormatContext **oc, AVStream *in_stream,
                            AVStream **out_stream, const char *output_dir, int index)
{
    int ret = 0;
    char filename[1024] = {'\0'};
    sprintf(filename, "%s/%s_%d.mp4", output_dir, TEMP_FILE_NAME, index);
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
    ret = avformat_write_header(*oc, NULL);
//    LOGI("avformat_write_header end ret = %d", ret);
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
    pkt->pts = av_rescale_q_rnd(pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    pkt->dts = av_rescale_q_rnd(dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
    pkt->pos = -1;
    ret = av_interleaved_write_frame(oc, pkt);
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

int split_video_by_gop(const char *input_file, const char *output_dir)
{
    LOGI("split video by gop start");
    int video_index = -1;
    int ret = 0, i = 0;
    int index = 0;
    AVFormatContext  *ic = NULL;
    AVFormatContext  *oc = NULL;
    AVStream  *in_stream = NULL;
    AVStream  *out_stream = NULL;
    AVPacket pkt;
//    LOGI("start split video %s to %s", input_file, output_dir);
    ret = avformat_open_input(&ic, input_file, NULL, NULL);
    if (ret != 0) {
        LOGI("open input format failed %s", av_err2str(ret));
        return -1;
    }
    ret = avformat_find_stream_info(ic, NULL);
    if (ret != 0) {
        avformat_close_input(&ic);
        return -1;
    }
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
//                LOGI("read key frame at pts %ld, dts %ld", pkt.pts, pkt.dts);
                close_write_item(&oc);
                ret = start_write_item(&oc, in_stream, &out_stream, output_dir, index++);
                if (ret < 0)
                    break;
                last_pts = pkt.pts;
                last_dts = pkt.dts;
            }
            int64_t pts = pkt.pts - last_pts;
            int64_t dts = pkt.dts - last_pts;
            ret = write_item_packet(oc, in_stream, out_stream, &pkt, pts, dts);
//            if (ret < 0)
//                break;
        }
        av_packet_unref(&pkt);
    }
    close_write_item(&oc);
    if (ic != NULL)
        avformat_close_input(&ic);
    LOGI("split video by gop done");
    return index;
}

static void video_work_thread(void *arg);

static void video_render_thread(void *arg);

static void notify_simple1(RPlayer *player, int what)
{
    msg_queue_put_simple1(&player->msg_q, what);
}

static void notify_simple2(RPlayer *player, int what, int arg1)
{
    msg_queue_put_simple2(&player->msg_q, what, arg1);
}

static void notify_simple3(RPlayer *player, int what, int arg1, int arg2)
{
    msg_queue_put_simple3(&player->msg_q, what, arg1, arg2);
}

static void rplayer_remove_msg(RPlayer *player, int what)
{
    msg_queue_remove(&player->msg_q, what);
}

static void rplayer_change_state_l(RPlayer *player, int state)
{
    player->state = state;
    notify_simple2(player, MSG_PLAYER_STATE_CHANGED, player->state);
}


RPlayer *rplayer_create(int(*msg_loop)(void*))
{
    RPlayer *player = (RPlayer *)malloc(sizeof(RPlayer));
    memset(player, 0, sizeof(RPlayer));
    player->msg_loop = msg_loop;
    return player;
}

void *rplayer_set_week_thiz(RPlayer *player, void *weak_thiz)
{
    void *week_thiz_pre = player->weak_thiz;
    player->weak_thiz = weak_thiz;
    return week_thiz_pre;
}

void *rplayer_get_week_thiz(RPlayer *player)
{
    return player->weak_thiz;
}

int rplayer_get_msg(RPlayer *player, AVMessage *msg, int block)
{
    while(1) {
        int ret = msg_queue_get(&player->msg_q, msg, block);
//        LOGI("%s, get msg from msg queue, ret = %d", __func__ , ret);
        if (ret < 0)
            return ret;
        switch (msg->what) {
            case MSG_PREPARED:
                pthread_mutex_lock(&player->mutex);
                if (player->state == MP_STATE_ASYNC_PREPARING) {
                    rplayer_change_state_l(player, MP_STATE_PREPARED);
                } else {
                    LOGD("MSG_PREPARED: expecting mp_state==MP_STATE_ASYNC_PREPARING\n");
                }
                pthread_mutex_unlock(&player->mutex);
                break;
            case MSG_COMPLETE:
                pthread_mutex_lock(&player->mutex);
                rplayer_change_state_l(player, MP_STATE_COMPLETED);
                pthread_mutex_unlock(&player->mutex);
                break;
            case MSG_ERROR:
                pthread_mutex_lock(&player->mutex);
                rplayer_change_state_l(player, MP_STATE_ERROR);
                pthread_mutex_unlock(&player->mutex);
                break;
        }
        return ret;
    }
    return -1;
}

int rplayer_init(RPlayer *player, ANativeWindow *window, int window_width, int window_height)
{
    LOGI("%s start", __func__ );
    pthread_mutex_init(&player->mutex, NULL);
    pthread_cond_init(&player->cond, NULL);
    msg_queue_init(&player->msg_q);
    player->window = window;
    player->window_width = window_width;
    player->window_height = window_height;
    player->abort_request = 0;
    player->pause_req = 0;
    player->seek_req = 0;
    player->seek_index = -1;
    player->seek_frame_offset = -1;
    player->segment_count = 0;
    LOGI("%s end", __func__ );
}

int rplayer_set_data_source(RPlayer *player, const char *path, const char *temp_dir)
{
    LOGI("%s start", __func__ );
    pthread_mutex_lock(&player->mutex);
    player->path = av_strdup(path);
    player->temp_dir = av_strdup(temp_dir);
    player->state = MP_STATE_INITIALIZED;
    pthread_mutex_unlock(&player->mutex);
    LOGI("%s end", __func__ );
    return 0;
}

static int init_segments(RPlayer *player)
{
    int ret, i, j;
    player->segments = (Segment *)malloc(sizeof(Segment) * player->segment_count);
    if (!player->segments) {
        LOGE("init segment failed...");
        return -1;
    }
    int index = 0;
    int64_t start_time = 0;
    for (i = player->segment_count -1; i >= 0; i--) {
        Segment *segment = (player->segments + index);
        AVFormatContext *fmt_ctx = NULL;
        int video_index = -1;
        sprintf(segment->mp4_path, "%s/%s_%d.mp4", player->temp_dir, TEMP_FILE_NAME, i);
        ret = avformat_open_input(&fmt_ctx, segment->mp4_path, NULL, NULL);
        if (ret != 0) {
            LOGE("open file %s failed, reason %s", segment->mp4_path, av_err2str(ret));
            ret = -2;
            break;
        }
        avformat_find_stream_info(fmt_ctx, NULL);
        for (j = 0; j < fmt_ctx->nb_streams; j++) {
            AVStream *stream = fmt_ctx->streams[j];
            if (AVMEDIA_TYPE_VIDEO == stream->codecpar->codec_type) {
                video_index = j;
                break;
            }
        }
        if (video_index < 0) {
            ret = -3;
            LOGE("No found video stream");
            break;
        }
        AVStream *vs = fmt_ctx->streams[video_index];
        AVRational frame_rate = vs->avg_frame_rate;
        if (frame_rate.den != 0 && frame_rate.num != 0) {
            segment->frame_show_time_ms = 1000 / (frame_rate.num / frame_rate.den);
        } else {
            segment->frame_show_time_ms = 40;
        }
        sprintf(segment->yuv_path, "%s/%s_%d.yuv", player->temp_dir, TEMP_FILE_NAME, i);
        segment->start_time = start_time;
        segment->width = vs->codecpar->width;
        segment->height = vs->codecpar->height;
        segment->duration = fmt_ctx->duration;
        segment->exist = 0;
        segment->frames = 0;
        segment->index = index;
        index++;
        start_time += segment->duration;
        avformat_close_input(&fmt_ctx);
    }
    player->duration = start_time;
    return ret;
}

/**
 * 工作线程（生产者）
 * 1、先把视频按gop拆成多个小的mp4
 * 2、不断把小的mp4 解码生成yuv文件
 * 3、把解码好的yuv文件
 * @param arg
 */
static void video_work_thread(void *arg)
{
    LOGI("video work thread started...");
    int ret, i = 0;
    int retval;
    RPlayer *player = (RPlayer *)arg;
    // 将MP4拆分
    LOGI("split video[%s] into %s start", player->path, player->temp_dir);
    retval = split_video_by_gop(player->path, player->temp_dir);
    LOGI("split video[%s] into %s complete, video_count is %d", player->path, player->temp_dir, retval);
    if (retval <= 0) {
        LOGE("split video failed exit work thread", player->path, player->temp_dir);
        notify_simple2(player, MSG_ERROR, ERROR_OPEN_INPUT_FAILED);
        return;
    }
    player->segment_count = retval;
    ret = init_segments(player);
    if (ret < 0) {
        LOGE("init gop segment failed...");
        notify_simple2(player, MSG_ERROR, ERROR_PREPARE_SEGMENT_FAILED);
        return;
    }
    int max_decodec_gop = FFMIN(player->segment_count, 4);
    // 开始生产yuv
    Segment *segment = NULL;
    int index = -1;
    for (;;) {
        if (player->abort_request)
            break;
        if (!player->seek_req && (player->pause_req || player->segment_q.size >= max_decodec_gop)) { // gop解码的足够多的时候等着消费一段时间
            usleep(20);
            continue;
        }
        LOGI("player seek req %d player seek to index %d, current decode index %d", player->seek_req, player->seek_index, index);
        if (player->seek_req) {
            index = player->seek_index;
            segment_queue_flush(&player->segment_q);
            player->seek_req = 0;
        } else {
            index = (index+1) % player->segment_count;
        }
        segment = player->segments + index;
        if (segment->exist) {
            LOGI("segment %d has exist, put segment into segment queue", index);
            segment_queue_put(&player->segment_q, segment);
            usleep(20);
            continue;
        } else {
            LOGI("start decode segment %d", index);
            int frame_count = decode_to_yuv420(segment->mp4_path, segment->yuv_path);
            segment->frames = frame_count;
            segment->exist = 1;
            segment_queue_put(&player->segment_q, segment);
            LOGI("start decode segment %d complete", index);
            // 解码出来了第一个gop 回调
            if (!player->prepared) {
                player->prepared = 1;
                notify_simple1(player, MSG_PREPARED);
            }
            if (player->video_width != segment->width ||
                player->video_height != segment->height) {
                player->video_width = segment->width;
                player->video_height = segment->height;
                LOGI("video width %d, video height %d", player->video_width, player->video_height);
                notify_simple3(player, MSG_VIDEO_SIZE_CHANGED, player->video_width,
                               player->video_height);
            }
        }
    }
    LOGE("work thread exit");
}

static void reset_seek_req(RPlayer *player)
{
    pthread_mutex_lock(&player->mutex);
    player->seek_req = 0;
    player->seek_index = -1;
    player->seek_frame_offset = -1;
    pthread_mutex_unlock(&player->mutex);
    notify_simple1(player, MSG_SEEK_COMPLETE);
}

static void reverse_render_yuv(RPlayer *player, Segment *segment) {
    LOGI("reverse_render_yuv %s", segment->yuv_path);
    FILE *file = NULL;
    file = fopen(segment->yuv_path, "rb");
    if (!file) {
        LOGE("open yuv file %s failed..", segment->yuv_path);
        return;
    }
    unsigned char *buffer[3];
    int32_t ysize = segment->width * segment->height;
    buffer[0] = malloc(sizeof(unsigned char) * ysize); // y
    buffer[1] = malloc(sizeof(unsigned char) * ysize / 4); // u
    buffer[2] = malloc(sizeof(unsigned char) * ysize / 4); // v
    int64_t frame_size = ysize + ysize / 4 + ysize / 4;
    size_t file_size = 0;
    fseek(file, 0, SEEK_END); // seek 到了末尾
    file_size = ftell(file);
    int64_t frame_count = file_size / frame_size;
    int frame_index = 0;
    LOGI("render segment %d, frame count %d,frame show time %ld", segment->index, frame_count, segment->frame_show_time_ms);
    while (file_size > 0 && frame_index < frame_count) {
        if (player->abort_request) {
            LOGE("abort exit render thread");
            break;
        }
        if (!player->seek_req && player->pause_req) {
            LOGE("pause wait start");
            usleep(20);
            continue;
        }
        if (player->seek_index >= 0 && segment->index != player->seek_index) {
            LOGI("has seek to need seek to index=%d, "
                 "current index=%d", player->seek_index, segment->index);
            break;
        }
        if (player->seek_frame_offset >= 0) {
            frame_index = player->seek_frame_offset;
        } else {
            frame_index++;
        }

        // 读取yuv数据
        int ret = 0;
        size_t read_size = 0;
        long seek_offset = -frame_index * frame_size;
        ret = fseek(file, seek_offset, SEEK_END);
//        LOGI("seek result %d, seek offset %ld", ret, seek_offset);
        read_size = fread(buffer[0], 1, ysize, file);  //  read y
        fread(buffer[1], 1, ysize / 4, file);  //  read u
        fread(buffer[2], 1, ysize / 4, file); //  read v
        if (feof(file) || read_size < 0) {
            LOGE("read file end");
            if (player->seek_req) {
                reset_seek_req(player);
            }
            break;
        }
        ret = gl_renderer_render(&player->renderer, buffer, segment->width, segment->height);
        if (ret < 0) {
            LOGE("gl render failed...");
            break;
        }
        if (segment->index == player->seek_index
            && frame_index == player->seek_frame_offset) {
            LOGI("reset seek req");
            reset_seek_req(player);
        }
        int64_t current_pos =
                segment->start_time / 1000 + frame_index * segment->frame_show_time_ms;
//        LOGI("current play pos %ld index=%d, frame_index = %d", current_pos, segment->index, frame_index);
        notify_simple3(player, MSG_PLAYER_POSITION_CHANGED, current_pos, player->duration / 1000);
        if (segment->index >= player->segment_count) {
            notify_simple1(player, MSG_COMPLETE);
        }
        if (!player->first_frame_rendered) {
            LOGI("start to render first frame");
            notify_simple1(player, MSG_RENDER_FIRST_FRAME);
            player->first_frame_rendered = 1;
        }
//        LOGI("sleep time %ld us", segment->frame_show_time_ms * 1000)
        usleep(segment->frame_show_time_ms * 1000);
    }
    free(buffer[0]);
    free(buffer[1]);
    free(buffer[2]);
    fclose(file);
}



/**
 * 渲染线程（消费者）
 *
 * @param arg
 */
static void video_render_thread(void *arg)
{
    LOGI("yuv render thread started");
    RPlayer *player = (RPlayer *)arg;
    gl_renderer_init(&player->renderer, player->window, player->window_width, player->window_height);
    int got_segment;
    Segment segment;
    for (;;) {
        if (player->abort_request)
            break;
        got_segment = segment_queue_get(&player->segment_q, &segment, 1);
        if (got_segment < 0) {
            break;
        }
        if (player->seek_index >= 0 && segment.index != player->seek_index) {
            LOGI("got segment %d, we need segment %d", segment.index, player->seek_index);
            continue;
        }
        reverse_render_yuv(player, &segment);
    }
    LOGI("video render thread exit");
}

static void message_loop_thread(void *arg)
{
    LOGI("msg loop thread start");
    RPlayer *player = (RPlayer *)arg;
    player->msg_loop(player);
    LOGI("msg loop thread exit...");
}


static int rplayer_prepare_l(RPlayer *player)
{
    LOGI("%s", __func__);
    rplayer_change_state_l(player, MP_STATE_ASYNC_PREPARING);
    msg_queue_start(&player->msg_q);
    segment_queue_init(&player->segment_q);
    // 开启一个消息线程，不断读取消息队列，post消息到上层
    pthread_create(&player->msg_loop_th, NULL, message_loop_thread, player);
    // 开启一个解码线程，按gop切分视频，解码成YUV文件
    pthread_create(&player->video_work_th, NULL, video_work_thread, player);
    // 开启一个视频渲染线程, 倒序渲染YUV文件
    pthread_create(&player->video_render_th, NULL, video_render_thread, player);
    return 0;
}


int rplayer_prepare(RPlayer *player)
{
    LOGI("%s", __func__ );
    int ret;
    pthread_mutex_lock(&player->mutex);
    ret = rplayer_prepare_l(player);
    pthread_mutex_unlock(&player->mutex);
    return ret;
}

int rplayer_start(RPlayer *player)
{
    LOGI("%s start", __func__ );
    pthread_mutex_lock(&player->mutex);
    player->pause_req = 0;
    pthread_mutex_unlock(&player->mutex);
    return 0;
}

int rplayer_seek_l(RPlayer *player, int64_t posUs)
{
    LOGI("rplayer_seek_l");
    if (!player->segments) {
        LOGE("rplayer_seek_l wait to init segment queue");
        return -1;
    }
    int i;
    for (i = 0; i < player->segment_count; i++) {
        Segment *segment = player->segments + i;
        if (posUs >= segment->start_time &&
            (posUs < segment->start_time + segment->duration)) {
            player->seek_req = 1;
            player->seek_index = i;
            player->seek_frame_offset = (posUs - segment->start_time) / (segment->frame_show_time_ms * 1000) + 1; // offset是从1开始的
            player->pause_req = 1;
            rplayer_change_state_l(player, MP_STATE_PAUSED);
            LOGI("seek to segment index = %d, frame offset = %d", player->seek_index, player->seek_frame_offset)
            break;
        }
    }
}

int rplayer_seek(RPlayer *player, int64_t posUs) // us
{
    int ret;
    LOGI("rplayer_seek posUs %ld", posUs);
    if (posUs < 0) {
        return -1;
    }
    pthread_mutex_lock(&player->mutex);
    ret = rplayer_seek_l(player, posUs);
    pthread_mutex_unlock(&player->mutex);
    return ret;
}

int rplayer_pause(RPlayer *player)
{
    LOGI("%s", __func__ );
    pthread_mutex_lock(&player->mutex);
    player->pause_req = 1;
    pthread_mutex_unlock(&player->mutex);
}

int rplayer_stop(RPlayer *player)
{
    LOGI("%s", __func__ );
    pthread_mutex_lock(&player->mutex);
    player->abort_request = 1;
    rplayer_change_state_l(player, MP_STATE_STOPPED);
    msg_queue_abort(&player->msg_q);
    segment_queue_abort(&player->segment_q);
    pthread_mutex_unlock(&player->mutex);
}

int rplayer_release(RPlayer *player)
{
    LOGI("%s", __func__ );
    pthread_join(player->msg_loop_th, NULL);
    msg_queue_destroy(&player->msg_q);
    pthread_join(player->video_work_th, NULL);
    pthread_join(player->video_render_th, NULL);
    gl_renderer_destroy(&player->renderer);
    av_free(player->path);
    av_free(player->temp_dir);
}

