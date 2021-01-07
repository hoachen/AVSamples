//
// Created by chenhao on 12/29/20.
//

#ifndef AVSAMPLES_RPLAYER_H
#define AVSAMPLES_RPLAYER_H

#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <libavutil/avutil.h>
#include <android/native_window_jni.h>
#include "video_split.h"
#include "video_convert_yuv.h"
#include "gl_renderer.h"
#include "segment_queue.h"
#include "message_queue.h"
#include "msg_def.h"

typedef struct RPlayer {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    const char *path;
    const char *temp_dir;
    ANativeWindow *window;
    int video_width;
    int video_height;
    int prepared;
    int window_width;
    int window_height;
    pthread_t msg_loop_th;
    pthread_t video_work_th;
    pthread_t video_render_th;
    int segment_count;
    GLRenderer renderer;
    SegmentQueue yuv_segment_q;
    int (* msg_loop)(void *);
    void *weak_thiz;
    MessageQueue msg_q;
    int abort_request;
    int start;
} RPlayer;


RPlayer *rplayer_create(int (*msg_loop)(void*));

void * rplayer_set_week_thiz(RPlayer *rp, void *week_obj);

void *rplayer_get_week_thiz(RPlayer *rp);

int rplayer_get_msg(RPlayer *rp, AVMessage *msg, int block);

int rplayer_init(RPlayer *player, ANativeWindow *window, int window_width, int window_height);

int rplayer_set_data_source(RPlayer *player, const char *path, const char *temp_dir);

int rplayer_prepare(RPlayer *player);

int rplayer_start(RPlayer *player);

int rplayer_seek(RPlayer *player, int64_t pos);

int rplayer_pause(RPlayer *player);

int rplayer_stop(RPlayer *player);

int rplayer_release(RPlayer *player);

#endif //AVSAMPLES_RPLAYER_H
