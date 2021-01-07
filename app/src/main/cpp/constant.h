//
// Created by chenhao on 1/6/21.
//

#ifndef AVSAMPLES_CONSTANT_H
#define AVSAMPLES_CONSTANT_H


#define MSG_FLUSH 0

#define MSG_ERROR    100
#define MSG_PREPARED 101
#define MSG_COMPLETE 102

#define  PLAYER_STATE_IDLE  0        // The player does not have any media to play.
#define  PLAYER_STATE_INITIALIZED  1      // The player sdk initialized
#define  PLAYER_STATE_BUFFERING  2   // The player has been buffering
#define  PLAYER_STATE_PREPARING  3    // The player start preparing
#define  PLAYER_STATE_PREPARED  4    // The player has been prepared
#define  PLAYER_STATE_STARTED  5     // start to play
#define  PLAYER_STATE_PAUSED  6//50;      // //The player pause playing the media.
#define  PLAYER_STATE_STOPPED  7//60;     // The player has been stop
#define  PLAYER_STATE_COMPLETED  8//70;    // The player has finished playing current media.
#define  PLAYER_STATE_RELEASED  9    //the player has been released resource
#define  PLAYER_STATE_ERROR  10    //the player occur error
#define  PLAYER_STATE_PLAYING  40    // The player has render the first frame



#endif //AVSAMPLES_CONSTANT_H
