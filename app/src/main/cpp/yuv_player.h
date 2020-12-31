//
// Created by chenhao on 12/31/20.
//

#ifndef AVSAMPLES_YUV_PLAYER_H
#define AVSAMPLES_YUV_PLAYER_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <android/native_window_jni.h>
#include <pthread.h>
#include "gl_renderer.h"
#include "log.h"

typedef struct YUVPlayer {
    const char *url;
    int video_width;
    int video_height;
    int window_width;
    int window_height;
    FILE *file;
    GLRenderer *renderer;
    ANativeWindow *window;
    int abort_request;
    int start;
    pthread_t video_render_th;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} YUVPlayer;

int yuv_player_init(YUVPlayer *player, ANativeWindow *window,  int window_width, int window_height,
        int video_width, int video_height, const char *url);

int yuv_player_prepare(YUVPlayer *player);

int yuv_player_start(YUVPlayer *player);

int yuv_player_stop(YUVPlayer *player);

int yuv_player_release(YUVPlayer *player);

#endif //AVSAMPLES_YUV_PLAYER_H
