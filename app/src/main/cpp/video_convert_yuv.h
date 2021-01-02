//
// Created by chenhao on 12/30/20.
//

#ifndef AVSAMPLES_VIDEO_CONVERT_YUV_H
#define AVSAMPLES_VIDEO_CONVERT_YUV_H

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

typedef struct VideoInfo {
    int64_t *duration;
    int video_width;
    int video_height;
    AVRational frame_rate;
} VideoInfo;


int convert_to_yuv420(const char *input_file, const char *output_file, VideoInfo *info);


#endif //AVSAMPLES_VIDEO_CONVERT_YUV_H
