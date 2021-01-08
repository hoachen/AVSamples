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
    int window_width;
    int window_height;
    int segment_count;
    GLRenderer renderer;
    SegmentQueue yuv_segment_q;
    int (* msg_loop)(void *);
    void *weak_thiz;
    MessageQueue msg_q;
    int abort_request;
    int pause_req;
    int prepared;
    int first_frame_rendered;
    int state;
    pthread_t msg_loop_th;
    pthread_t video_work_th;
    pthread_t video_render_th;
} RPlayer;

/*-
 * ijkmp_set_data_source()  -> MP_STATE_INITIALIZED
 *
 * ijkmp_reset              -> self
 * ijkmp_release            -> MP_STATE_END
 */
#define MP_STATE_IDLE               0

/*-
 * ijkmp_prepare_async()    -> MP_STATE_ASYNC_PREPARING
 *
 * ijkmp_reset              -> MP_STATE_IDLE
 * ijkmp_release            -> MP_STATE_END
 */
#define MP_STATE_INITIALIZED        1

/*-
 *                   ...    -> MP_STATE_PREPARED
 *                   ...    -> MP_STATE_ERROR
 *
 * ijkmp_reset              -> MP_STATE_IDLE
 * ijkmp_release            -> MP_STATE_END
 */
#define MP_STATE_ASYNC_PREPARING    2

/*-
 * ijkmp_seek_to()          -> self
 * ijkmp_start()            -> MP_STATE_STARTED
 *
 * ijkmp_reset              -> MP_STATE_IDLE
 * ijkmp_release            -> MP_STATE_END
 */
#define MP_STATE_PREPARED           3

/*-
 * ijkmp_seek_to()          -> self
 * ijkmp_start()            -> self
 * ijkmp_pause()            -> MP_STATE_PAUSED
 * ijkmp_stop()             -> MP_STATE_STOPPED
 *                   ...    -> MP_STATE_COMPLETED
 *                   ...    -> MP_STATE_ERROR
 *
 * ijkmp_reset              -> MP_STATE_IDLE
 * ijkmp_release            -> MP_STATE_END
 */
#define MP_STATE_STARTED            4

/*-
 * ijkmp_seek_to()          -> self
 * ijkmp_start()            -> MP_STATE_STARTED
 * ijkmp_pause()            -> self
 * ijkmp_stop()             -> MP_STATE_STOPPED
 *
 * ijkmp_reset              -> MP_STATE_IDLE
 * ijkmp_release            -> MP_STATE_END
 */
#define MP_STATE_PAUSED             5

/*-
 * ijkmp_seek_to()          -> self
 * ijkmp_start()            -> MP_STATE_STARTED (from beginning)
 * ijkmp_pause()            -> self
 * ijkmp_stop()             -> MP_STATE_STOPPED
 *
 * ijkmp_reset              -> MP_STATE_IDLE
 * ijkmp_release            -> MP_STATE_END
 */
#define MP_STATE_COMPLETED          6

/*-
 * ijkmp_stop()             -> self
 * ijkmp_prepare_async()    -> MP_STATE_ASYNC_PREPARING
 *
 * ijkmp_reset              -> MP_STATE_IDLE
 * ijkmp_release            -> MP_STATE_END
 */
#define MP_STATE_STOPPED            7

/*-
 * ijkmp_reset              -> MP_STATE_IDLE
 * ijkmp_release            -> MP_STATE_END
 */
#define MP_STATE_ERROR              8

/*-
 * ijkmp_release            -> self
 */
#define MP_STATE_END                9

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
