//
// Created by chenhao on 12/29/20.
//

#include "rplayer.h"

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
        LOGI("%s, get msg from msg queue, ret = %d", __func__ , ret);
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
    player->pause_req = 1;
    player->segment_count = 0;
    LOGI("%s end", __func__ );
}

int rplayer_set_data_source(RPlayer *player, const char *path, const char *temp_dir)
{
    LOGI("%s start", __func__ );
    pthread_mutex_lock(&player->mutex);
    player->path = av_strdup(path);
    player->temp_dir = av_strdup(temp_dir);
    rplayer_change_state_l(player, MP_STATE_INITIALIZED);
    pthread_mutex_unlock(&player->mutex);
    LOGI("%s end", __func__ );
    return 0;
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
    int i = 0;
    int retval;
    RPlayer *player = (RPlayer *)arg;

    // 将MP4拆分
    LOGI("split video[%s] into %s start", player->path, player->temp_dir);
    retval = split_video_by_gop(player->path, player->temp_dir);
    LOGI("split video[%s] into %s complete, video_count is %d", player->path, player->temp_dir, retval);
    if (retval <= 0) {
        LOGE("split video failed exit work thread", player->path, player->temp_dir);
        notify_simple2(player, MSG_ERROR, retval);
        return;
    }
    player->segment_count = retval;
    segment_queue_init(&player->yuv_segment_q);
    int frame_count = 0;
    LOGI("start to convet yuv file");

    for (i = retval - 1; i >= 0; i--) {
        if (player->abort_request)
            break;
        char mp4_file_path[1024] = {'\0'};
        sprintf(mp4_file_path, "%s/%s_%d.mp4", player->temp_dir, TEMP_FILE_NAME, i);

        char yuv_file_path[1024] = {'\0'};
        sprintf(yuv_file_path, "%s/%s_%d.yuv", player->temp_dir, TEMP_FILE_NAME, i);
        VideoInfo info;
//        LOGI("convert %s to %s start", mp4_file_path, yuv_file_path);
        frame_count = convert_to_yuv420(mp4_file_path, yuv_file_path, &info);
        LOGI("convert %s to %s end frame count is %d", mp4_file_path, yuv_file_path, frame_count);
        if (frame_count <= 0)
            continue;
        // 解码出来了第一个gop 回调
        if (!player->prepared) {
            player->prepared = 1;
            notify_simple1(player, MSG_PREPARED);
        }
        if (player->video_width != info.video_width || player->video_height != info.video_height) {
            player->video_width = info.video_width;
            player->video_height = info.video_height;
            notify_simple3(player, MSG_VIDEO_SIZE_CHANGED, player->video_width, player->video_height);
        }
        LOGI("frame rate %d %d", info.frame_rate.num, info.frame_rate.den);
        int64_t frame_show_time_ms = 1000 / (info.frame_rate.num / info.frame_rate.den);
        segment_queue_put(&player->yuv_segment_q, yuv_file_path, info.video_width,
                          info.video_height, frame_count, info.duration, frame_show_time_ms);
    }
    LOGE("work thread exit");
}

static void reverse_render_yuv(RPlayer *player, YUVSegment *segment)
{
    FILE *file = NULL;
    file = fopen(segment->path, "rb");
    if (!file) {
        LOGE("open yuv file %s failed..", segment->path);
        return;
    }
    unsigned char *buffer[3];
    int32_t ysize = segment->width * segment->height;
    buffer[0] = malloc( sizeof(unsigned char) * ysize); // y
    buffer[1] = malloc(sizeof(unsigned char) * ysize / 4); // u
    buffer[2] = malloc(sizeof(unsigned char) * ysize / 4); // v
    int64_t frame_size = ysize  + ysize / 4 + ysize / 4;
    size_t file_size = 0;
    fseek(file, 0, SEEK_END); // seek 到了末尾
    file_size = ftell(file);
    int64_t frame_count = file_size / frame_size;
    int frame_index = 0;
    LOGI("a frame size is %ld, file_size is %ld, frame count %d, frame show time %ld", frame_size, file_size, frame_count, segment->frame_show_time_ms);
    do {
        if (player->abort_request) {
            LOGE("abort exit render thread");
            break;
        }
        if (player->pause_req) {
            LOGE("pause wait start");
            usleep(20);
            continue;
        }
        if (!player->first_frame_rendered) {
            LOGI("start to render first frame");
            notify_simple1(player, MSG_RENDER_FIRST_FRAME);
            player->first_frame_rendered = 1;
        }
        // 读取yuv数据
        int ret = 0;
        size_t read_size = 0;
        frame_index++;
        long seek_offset = -frame_index * frame_size;
        ret = fseek(file, seek_offset, SEEK_END);
        LOGI("seek result %d, seek offset %ld", ret, seek_offset);
        read_size = fread(buffer[0], 1, ysize,file); // y
        read_size = fread(buffer[1], 1, ysize / 4, file); // u
        fread(buffer[2], 1, ysize / 4 , file); // v
        ret = gl_renderer_render(&player->renderer, buffer, segment->width, segment->height);
        if (ret < 0) {
            LOGE("gl render failed...");
            break;
        }
        if (feof(file) || read_size < 0) {
            LOGE("read file end");
            break;
        }
        LOGI("sleep time %ld us", segment->frame_show_time_ms * 1000)
        usleep(segment->frame_show_time_ms * 1000);
    } while (frame_index < frame_count);
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
    YUVSegment segment;
    int got_segment = 0;
    int render_count = 0;
    for (;;) {
        if (player->abort_request)
            break;
        got_segment = segment_queue_get(&player->yuv_segment_q, &segment, 1);
        LOGI("CHHH start render yuv");
        if (got_segment) {
            reverse_render_yuv(player, &segment);
            render_count++;
        } else if (got_segment < 0) {
            LOGE("get segment from queue failed...");
            break;
        }
        LOGI("has render segment count %d", render_count);
        if (render_count >= player->segment_count) {
            notify_simple1(player, MSG_COMPLETE);
            break;
        }
    }
    LOGI("video render thread exit");
}

static void message_loop_thread(void *arg)
{
    RPlayer *player = (RPlayer *)arg;
    player->msg_loop(player);
    LOGI("msg loop thread exit...");
}


static int rplayer_prepare_l(RPlayer *player)
{
    LOGI("%s", __func__);
    rplayer_change_state_l(player, MP_STATE_ASYNC_PREPARING);
    msg_queue_start(&player->msg_q);
    pthread_create(&player->msg_loop_th, NULL, message_loop_thread, player);
    // 开启一个解封装线程
    pthread_create(&player->video_work_th, NULL, video_work_thread, player);
    // 开启一个视频渲染线程
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
    if (player->state != MP_STATE_PREPARED)
        return -1;
    player->pause_req = 0;
    pthread_cond_signal(&player->cond);
    pthread_mutex_unlock(&player->mutex);
    return 0;
}


int rplayer_seek(RPlayer *player, int64_t pos)
{
    LOGI("%s", __func__ );

}

int rplayer_pause(RPlayer *player)
{
    LOGI("%s", __func__ );
    pthread_mutex_lock(&player->mutex);
    player->pause_req = 0;
    pthread_cond_signal(&player->cond);
    pthread_mutex_unlock(&player->mutex);
}

int rplayer_stop(RPlayer *player)
{
    LOGI("%s", __func__ );
    pthread_mutex_lock(&player->mutex);
    player->abort_request = 1;
    segment_queue_abort(&player->yuv_segment_q);
    msg_queue_abort(&player->msg_q);
    pthread_mutex_unlock(&player->mutex);
}

int rplayer_release(RPlayer *player)
{
    LOGI("%s", __func__ );
    pthread_join(player->msg_loop_th, NULL);
    msg_queue_destroy(&player->msg_q);
    pthread_join(player->video_work_th, NULL);
    segment_queue_destroy(&player->yuv_segment_q);
    pthread_join(player->video_render_th, NULL);
    gl_renderer_destroy(&player->renderer);
    av_free(player->path);
    av_free(player->temp_dir);
}

