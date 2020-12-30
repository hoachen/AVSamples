//
// Created by chenhao on 12/29/20.
//

#ifndef AVSAMPLES_XPLAYER_H
#define AVSAMPLES_XPLAYER_H

#include <stdint.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <pthread.h>
#include "video_split.h"


typedef struct XPlayer {
    pthread_mutex_t *mutex;
    pthread_cond_t *mutex_cond;
    const char *path;

} XPlayer;


void player_init(XPlayer *player);

int player_set_data_source(XPlayer *player, const char *path);

int player_prepare(XPlayer *player);

int player_start(XPlayer *player);

int player_seek_to(XPlayer *player, int64_t pos);

int player_pause(XPlayer *player);

int player_stop(XPlayer *player);

int player_release(XPlayer *player);

#endif //AVSAMPLES_XPLAYER_H
