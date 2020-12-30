//
// Created by chenhao on 12/29/20.
//

#include "xplayer.h"


void player_init(XPlayer *player)
{
    pthread_mutex_init(player->mutex, NULL);
    pthread_cond_init(player->mutex_cond, NULL);
}

int player_set_data_source(XPlayer *player, const char *path)
{
    pthread_mutex_lock(player->mutex);
    player->path = av_strdup(path);
    pthread_mutex_unlock(player->mutex);
}

int player_prepare_l(XPlayer *player)
{

}

int player_prepare(XPlayer *player)
{
    pthread_mutex_lock(player->mutex);
    player_prepare_l(player);
    pthread_mutex_unlock(player->mutex);
}
