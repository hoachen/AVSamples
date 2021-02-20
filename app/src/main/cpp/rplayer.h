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
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <sys/statfs.h>
#include "gl_renderer.h"
#include "message_queue.h"
#include "msg_def.h"


typedef struct Segment
{
    char mp4_path[1024];
    char yuv_path[1024];
    int width;
    int height;
    int frames;
    int exist;
    int index;
    int64_t start_time;
    int64_t frame_show_time_ms;
    int64_t duration;
} Segment;

typedef struct SegmentNode {
    struct Segment segment;
    struct SegmentNode *next;
} SegmentNode;

typedef struct SegmentQueue
{
    SegmentNode *head, *tail;
    int size;
    int64_t frame_count;
    int64_t duration;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int abort_request;
} SegmentQueue;

typedef struct RPlayer {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    const char *path;
    const char *temp_dir;
    ANativeWindow *window;    //
    int video_width;          // video width
    int video_height;         // video height
    int window_width;         // display width
    int window_height;        // display height
    int64_t duration;         // video total duration
    int segment_count;
    GLRenderer renderer;      // open gl render
    SegmentQueue segment_q;   // gop segment queue
    Segment *segments;
    int (* msg_loop)(void *);
    void *weak_thiz;
    MessageQueue msg_q;
    int abort_request;
    int pause_req;
    int seek_req;
    int seek_index;
    int seek_frame_offset;          // found index and calculate offset
    int prepared;
    int first_frame_rendered;
    int state;
    pthread_t msg_loop_th;
    pthread_t video_work_th;
    pthread_t video_render_th;
} RPlayer;

#define MP_STATE_IDLE               0
#define MP_STATE_INITIALIZED        1
#define MP_STATE_ASYNC_PREPARING    2
#define MP_STATE_PREPARED           3
#define MP_STATE_STARTED            4
#define MP_STATE_PAUSED             5
#define MP_STATE_COMPLETED          6
#define MP_STATE_STOPPED            7
#define MP_STATE_ERROR              8


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
