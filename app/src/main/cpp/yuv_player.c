//
// Created by chenhao on 12/31/20.
//

#include <unistd.h>
#include "yuv_player.h"

static void video_render_thread(void *arg)
{
    YUVPlayer *player = (YUVPlayer *)arg;
    gl_renderer_init(player->renderer, player->window, player->window_width, player->window_height);
    LOGI("gl_renderer_init");
    unsigned char *buffer[3] = {0};
    int32_t ysize = player->video_width * player->video_height;
    buffer[0] = malloc(sizeof(ysize)); // y
    buffer[1] = malloc(sizeof(ysize / 4)); // y
    buffer[2] = malloc(sizeof(ysize / 4)); // y
    for (;;) {
        if (player->abort_request) {
            LOGE("has abort exit render thread");
            break;
        }
        pthread_mutex_lock(&player->mutex);
        if (!player->start) {
            LOGI("wait to call player start");
            pthread_cond_wait(&player->cond, &player->mutex);
        }
        pthread_mutex_unlock(&player->mutex);
        LOGI("start to render a frame");
        // 读取yuv数据
        size_t read_size = 0;
        read_size = fread(buffer[0], 1, ysize, player->file); // y
        LOGI("read y data size %d", read_size);
        read_size = fread(buffer[1], 1, ysize / 4, player->file); // u
        LOGI("read u data size %d", read_size);
        fread(buffer[2], 1, ysize / 4 ,player->file); // v
        LOGI("read v data size %d", read_size);
        int ret;
        ret = gl_renderer_render(player->renderer, buffer, player->video_width, player->video_height);
        if (ret < 0) {
            LOGE("gl render failed...");
        }
        if (feof(player->file) || read_size < 0) {
            LOGE("read file end");
            break;
        }
        usleep(40 * 1000);
    }
    free(buffer[0]);
    free(buffer[1]);
    free(buffer[2]);
}

int yuv_player_init(YUVPlayer *player, ANativeWindow *window, int window_width, int window_height,
             int video_width, int video_height, const char *url)
{
    int ret = 0;
    player->renderer = (GLRenderer *)malloc(sizeof(GLRenderer));
    memset(player->renderer, 0, sizeof(GLRenderer));
    player->window = window;
    player->url = strdup(url);
    player->window_width = window_width;
    player->window_height = window_height;
    player->video_width = video_width;
    player->video_height = video_height;
    player->abort_request = 0;
    player->start = 0;
    pthread_mutex_init(&player->mutex, NULL);
    pthread_cond_init(&player->cond, NULL);
    return ret;
}

static int yuv_player_prepare_l(YUVPlayer *player)
{
    LOGI("%s to open %s", __func__ , player->url);
    player->file = fopen(player->url, "rb");
    if (!player->file) {
        LOGE("open yuv file failed...");
        return -1;
    }
    pthread_create(&player->video_render_th, NULL, video_render_thread, player);
    LOGI("start video_render_thread");
    return 0;
}

int yuv_player_prepare(YUVPlayer *player)
{
    int ret = 0;
    pthread_mutex_lock(&player->mutex);
    ret = yuv_player_prepare_l(player);
    pthread_mutex_unlock(&player->mutex);
    return ret;
}

int yuv_player_start(YUVPlayer *player)
{
    LOGI("yuv_player_start")
    pthread_mutex_lock(&player->mutex);
    player->start = 1;
    pthread_cond_signal(&player->cond);
    pthread_mutex_unlock(&player->mutex);
}


int yuv_player_stop(YUVPlayer *player)
{
    pthread_mutex_lock(&player->mutex);
    player->abort_request = 1;
    pthread_mutex_unlock(&player->mutex);
}

int yuv_player_release(YUVPlayer *player)
{
    pthread_mutex_lock(&player->mutex);
    if (player->file)
        fclose(player->file);
    player->abort_request = 1;
    player->start = 0;
    gl_renderer_destroy(player->renderer);
    pthread_mutex_unlock(&player->mutex);
    pthread_cond_destroy(&player->cond);
    pthread_mutex_destroy(&player->mutex);
    free(player);
}
