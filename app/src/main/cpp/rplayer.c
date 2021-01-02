//
// Created by chenhao on 12/29/20.
//

#include "rplayer.h"

static void video_work_thread(void *arg);

static void video_render_thread(void *arg);

int rplayer_init(RPlayer *player, ANativeWindow *window, int window_width, int window_height)
{
    LOGI("%s start", __func__ );
    pthread_mutex_init(&player->mutex, NULL);
    pthread_cond_init(&player->cond, NULL);
    player->window = window;
    player->window_width = window_width;
    player->window_height = window_height;
    player->abort_request = 0;
    player->start = 0;
    player->segment_count = 0;
    LOGI("%s end", __func__ );
}

int rplayer_set_data_source(RPlayer *player, const char *path, const char *temp_dir)
{
    LOGI("%s start", __func__ );
    pthread_mutex_lock(&player->mutex);
    player->path = av_strdup(path);
    player->temp_dir = av_strdup(temp_dir);
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
    int video_count;
    RPlayer *rp = (RPlayer *)arg;

    // 将MP4拆分
    LOGI("split video[%s] into %s start", rp->path, rp->temp_dir);
    video_count = split_video(rp->path, rp->temp_dir);
    LOGI("split video[%s] into %s complete, video_count is %d", rp->path, rp->temp_dir, video_count);
    if (video_count <= 0) {
        LOGE("split video failed exit work thread", rp->path, rp->temp_dir);
        return;
    }
    rp->segment_count = video_count;
    segment_queue_init(&rp->yuv_segment_q);
    int frame_count = 0;
    LOGI("start to convet yuv file");
    for (i = video_count - 1; i >= 0; i--) {
        if (rp->abort_request)
            break;
        char mp4_file_path[1024] = {'\0'};
        sprintf(mp4_file_path, "%s/%s_%d.mp4", rp->temp_dir, TEMP_FILE_NAME, i);

        char yuv_file_path[1024] = {'\0'};
        sprintf(yuv_file_path, "%s/%s_%d.yuv", rp->temp_dir, TEMP_FILE_NAME, i);
        VideoInfo info;
//        LOGI("convert %s to %s start", mp4_file_path, yuv_file_path);
        frame_count = convert_to_yuv420(mp4_file_path, yuv_file_path, &info);
        LOGI("convert %s to %s end frame count is %d", mp4_file_path, yuv_file_path, frame_count);
        if (frame_count <= 0)
            continue;
        LOGI("frame rate %d %d", info.frame_rate.num, info.frame_rate.den);
        int64_t frame_show_time_ms = 1000 / (info.frame_rate.num / info.frame_rate.den);
        segment_queue_put(&rp->yuv_segment_q, yuv_file_path, info.video_width,
                          info.video_height, frame_count, info.duration, frame_show_time_ms);
    }
}

static int first_render = 0;

static void reverse_render_yuv(RPlayer *rp, YUVSegment *segment)
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
        if (rp->abort_request) {
            LOGE("has abort exit render thread");
            break;
        }
        if (!first_render) {
            LOGI("start to render first frame");
            first_render = 1;
        }
        // 读取yuv数据
        int ret = 0;
        size_t read_size = 0;
        frame_index++;
        long seek_offset = -frame_index * frame_size;
        ret = fseek(file, seek_offset, SEEK_END);
//        LOGI("seek result %d, seek offset %ld", ret, seek_offset);
        read_size = fread(buffer[0], 1, ysize,file); // y
//        LOGI("read y data size %d", read_size);
        read_size = fread(buffer[1], 1, ysize / 4, file); // u
//        LOGI("read u data size %d", read_size);
        fread(buffer[2], 1, ysize / 4 , file); // v
//        LOGI("read v data size %d", read_size);
        ret = gl_renderer_render(&rp->renderer, buffer, segment->width, segment->height);
        if (ret < 0) {
            LOGE("gl render failed...");
        }
        if (feof(file) || read_size < 0) {
            LOGE("read file end");
            break;
        }
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
    RPlayer *rp = (RPlayer *)arg;
    gl_renderer_init(&rp->renderer, rp->window, rp->window_width, rp->window_height);
    YUVSegment segment;
    int got_segment = 0;
    int render_count = 0;
    for (;;) {
        if (rp->abort_request)
            break;
        got_segment = segment_queue_get(&rp->yuv_segment_q, &segment, 1);
        LOGI("CHHH start render yuv");
        if (got_segment) {
            reverse_render_yuv(rp, &segment);
            render_count++;
        } else if (got_segment < 0) {
            LOGE("get segment from queue failed...");
            break;
        }
        LOGI("has render segment count %d", render_count);
        if (render_count >= rp->segment_count) {
            break;
        }
    }

}


static int rplayer_prepare_l(RPlayer *player)
{
    LOGI("%s start", __func__);
    // 开启一个解封装线程
    // 开启一个视频渲染线程
    pthread_create(&player->video_work_th, NULL, video_work_thread, player);
    pthread_create(&player->video_render_th, NULL, video_render_thread, player);
    LOGI("%s end", __func__ );
}


int rplayer_prepare(RPlayer *player)
{
    pthread_mutex_lock(&player->mutex);
    rplayer_prepare_l(player);
    pthread_mutex_unlock(&player->mutex);
}

int rplayer_start(RPlayer *player)
{
    LOGI("%s start", __func__ );
    pthread_mutex_lock(&player->mutex);
    player->start = 1;
    pthread_cond_signal(&player->cond);
    pthread_mutex_unlock(&player->mutex);
    LOGI("%s end", __func__ );
}


int rplayer_seek(RPlayer *player, int64_t pos)
{

}

int rplayer_pause(RPlayer *player)
{
    pthread_mutex_lock(&player->mutex);
    player->start = 0;
    pthread_cond_signal(&player->cond);
    pthread_mutex_unlock(&player->mutex);
}

int rplayer_stop(RPlayer *player)
{
    pthread_mutex_lock(&player->mutex);
    player->abort_request = 1;
    pthread_mutex_unlock(&player->mutex);
}

int rplayer_release(RPlayer *player)
{
    pthread_join(player->video_work_th, NULL);
    segment_queue_destroy(&player->yuv_segment_q);
    pthread_join(player->video_render_th, NULL);
    gl_renderer_destroy(&player->renderer);
    av_free(player->path);
    av_free(player->temp_dir);
}

